/*
 *
 *  GattLib - GATT Library
 *
 *  Copyright (C) 2016-2017 Olivier Martin <olivier@labapart.org>
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <glib.h>
#include <stdbool.h>
#include <stdlib.h>

#include "gattlib_internal.h"

#define CONNECT_TIMEOUT  4

int gattlib_adapter_open(const char* adapter_name, void** adapter) {
	char object_path[20];
	OrgBluezAdapter1 *adapter_proxy;
	GError *error = NULL;

	if (adapter_name) {
		snprintf(object_path, sizeof(object_path), "/org/bluez/%s", adapter_name);
	} else {
		strncpy(object_path, "/org/bluez/hci0", sizeof(object_path));
	}

	adapter_proxy = org_bluez_adapter1_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
			"org.bluez",
			object_path,
			NULL, &error);
	if (adapter_proxy == NULL) {
		//printf("Failed to get adapter %s\n", object_path);
		return -1;
	}

	// Ensure the adapter is powered on
	if(! org_bluez_adapter1_get_powered(adapter_proxy) )
		org_bluez_adapter1_set_powered(adapter_proxy, TRUE);

	*adapter = adapter_proxy;
	return 0;
}

static gboolean stop_scan_func(gpointer data) {
	g_main_loop_quit(data);
	return FALSE;
}


void gattlib_power_on_adapter(void* adapter)
{
	org_bluez_adapter1_set_powered(adapter, TRUE);
}

void gattlib_power_off_adapter(void* adapter)
{
	org_bluez_adapter1_set_powered(adapter, FALSE);
}
void on_dbus_object_added(GDBusObjectManager *device_manager,
                     GDBusObject        *object,
                     gpointer            user_data)
{
	const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));
	GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.Device1");
	if (!interface) {
		return;
	}

    GError *error = NULL;
	OrgBluezDevice1* device1 = org_bluez_device1_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			object_path,
			NULL,
			&error);

	if (device1) {
		gattlib_discovered_device_t discovered_device_cb = user_data;

		discovered_device_cb(
			org_bluez_device1_get_address(device1),
			org_bluez_device1_get_name(device1));
		g_object_unref(device1);
	}
}

int gattlib_set_discovery_filter(void* adapter)
{
	GError *error = NULL;
	GVariantBuilder *b = g_variant_builder_new(G_VARIANT_TYPE_VARDICT);
	g_variant_builder_add(b, "{sv}", "Transport", g_variant_new_string("le"));
	//g_variant_builder_add(b, "{sv}", "RSSI",g_variant_new_int16(-g_ascii_strtod("99", NULL)));
	GVariantBuilder *u = g_variant_builder_new(G_VARIANT_TYPE_STRING_ARRAY);
	g_variant_builder_add(u, "s", "ADE3D529-C784-4F63-A987-EB69F70EE816");
	g_variant_builder_add(b, "{sv}", "UUIDs", g_variant_builder_end(u));
	GVariant *device_dict = g_variant_builder_end(b);
	g_variant_builder_unref(u);
	g_variant_builder_unref(b);
	//printf("\n Setting filters");
	fflush(stdout);
	if (g_variant_n_children (device_dict) > 0) {
		GVariantIter *iter;
		const gchar *key;
		GVariant *value;

		g_variant_get (device_dict, "a{sv}", &iter);
		while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
			//char arr[100];
			//g_variant_get(value,arr);
			if(!strcmp("Transport",key))
				printf("\n  %s : %s",key,g_variant_get_string(value,NULL));
			else
			{
				g_variant_get_strv(value,NULL);
				//char **arr=g_variant_get_strv(value,NULL);
				//printf("\n%s : %s",key,arr[0]);
			}
		}
	}

	if(org_bluez_adapter1_call_set_discovery_filter_sync((OrgBluezAdapter1*)adapter,device_dict,NULL,&error) == FALSE)
	{
		//printf("\nFilter could not be set %s",error->message);
		return -1;
	}
	return 0;
}

int gattlib_adapter_scan_enable(void* adapter, gattlib_discovered_device_t discovered_device_cb, int timeout) {
	GDBusObjectManager *device_manager;
	GError *error = NULL;

	if(timeout <=0)
		timeout=1;

	org_bluez_adapter1_call_start_discovery_sync((OrgBluezAdapter1*)adapter, NULL, &error);
	if(error)
	{
		//printf("\nFailed to start discovery %s",error->message);
        return -1;
	}
	//
	// Get notification when objects are removed from the Bluez ObjectManager.
	// We should get notified when the connection is lost with the target to allow
	// us to advertise us again
	//
	device_manager = g_dbus_object_manager_client_new_for_bus_sync (
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/",
			NULL, NULL, NULL, NULL,
			&error);
	if (device_manager == NULL) {
		//printf("Failed to get Bluez Device Manager %s",error->message);
		return -1;
	}
	/* We are doing this becuase we may get false negative for discovered devices rssi.
	 * Give some time to update rssi for devices
	 * */
	sleep(2);

	GList *objects = g_dbus_object_manager_get_objects(device_manager);
	GList *l;
	for (l = objects; l != NULL; l = l->next)
	{
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.Device1");
		if (!interface) {
			continue;
		}

		error = NULL;
		OrgBluezDevice1* device1 = org_bluez_device1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);

		if (device1) {
			//printf("\n RSSI is %d",org_bluez_device1_get_rssi(device1));
			if(org_bluez_device1_get_rssi(device1) != 0)
			{
				discovered_device_cb(
					org_bluez_device1_get_address(device1),
					org_bluez_device1_get_name(device1));
			}
			g_object_unref(device1);
//			else if(!org_bluez_device1_get_connected(device1) && !org_bluez_device1_get_paired(device1))
//			{
//				// remove object if advertising device no longer exists which is not conn and paired
//				char address[20];
//				strcpy(address,org_bluez_device1_get_address(device1));
//				g_object_unref(device1);
//				printf("address is %s",address);
//				gattlib_remove_device(adapter_name,address);
//			}
		}
	}

	g_list_free_full(objects, g_object_unref);

	gulong handler_id = g_signal_connect (G_DBUS_OBJECT_MANAGER(device_manager),
	                    "object-added",
	                    G_CALLBACK (on_dbus_object_added),
	                    discovered_device_cb);

	// Run Glib loop for 'timeout' seconds
	GMainLoop *loop = g_main_loop_new(NULL, 0);
	g_timeout_add_seconds (timeout, stop_scan_func, loop);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	if (handler_id > 0)
		g_signal_handler_disconnect(G_DBUS_OBJECT_MANAGER(device_manager), handler_id);

	g_object_unref(device_manager);
	return 0;
}

int gattlib_adapter_scan_disable(void* adapter) {
	GError *error = NULL;

	if( org_bluez_adapter1_call_stop_discovery_sync((OrgBluezAdapter1*)adapter, NULL, &error) == FALSE)
	{
		//printf("\nDiscovery cannot be stopped : %s",error->message);
		return -1;
	}
	return 0;
}

int gattlib_adapter_close(void* adapter) {
	g_object_unref(adapter);
	return 0;
}


gboolean on_handle_device_property_change(
	    OrgBluezGattCharacteristic1 *object,
	    GVariant *arg_changed_properties,
	    const gchar *const *arg_invalidated_properties,
	    gpointer user_data)
{
	GMainLoop *loop = user_data;

	// Retrieve 'Value' from 'arg_changed_properties'
	if (g_variant_n_children (arg_changed_properties) > 0) {
		GVariantIter *iter;
		const gchar *key;
		GVariant *value;

		g_variant_get (arg_changed_properties, "a{sv}", &iter);
		while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
			if (strcmp(key, "UUIDs") == 0) {
				g_main_loop_quit(loop);
				break;
			}
		}
	}
	return TRUE;
}

gboolean on_handle_device_property_change_connection(
	    OrgBluezGattCharacteristic1 *object,
	    GVariant *arg_changed_properties,
	    const gchar *const *arg_invalidated_properties,
	    gpointer user_data)
{
	gatt_connection_t* connection = user_data;
	// [TODO] add null check also check that this connection is still connected
	gattlib_context_t* conn_context = connection->context;
	//printf("[GATTLIB]Entered  %s\n",__func__);
	// Retrieve 'Value' from 'arg_changed_properties'
	if (g_variant_n_children (arg_changed_properties) > 0) {
		GVariantIter *iter;
		const gchar *key;
		GVariant *value;

		g_variant_get (arg_changed_properties, "a{sv}", &iter);
		while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
			//printf("[GATTLIB]Value changed = %s\n", key);
			//printf("[GATTLIB]Value changed type= %s", g_variant_get_type_string (value));
			int case_compare = strcasecmp (key, "Connected");
			//printf("[GATTLIB]Case compare = %d\n", case_compare);

			if ((case_compare  == 0)) { //0 &&
				gboolean connected_value = g_variant_get_boolean (value);
				//printf("[GATTLIB]Value changed value= %d\n", connected_value);
				if (!connected_value && conn_context->disconnect_cb != NULL ){
					//printf("[GATTLIB]Value changed calling callback %d\n", connected_value);
					conn_context->disconnect_cb(connection);
				}
				else
				{
					//printf("[GATTLIB]No callback %d\n", connected_value);
				}
			//	g_main_loop_quit(loop);
			//	break;
			}
			else
			{
				//printf("[GATTLIB]Value changed comparison failed %s\n", key);
			}
		}
	}
	//printf("[GATTLIB]left  %s\n",__func__);

	return TRUE;
}

int gattlib_register_disconnect(gatt_connection_t* connection, gatt_disconnect_cb_t disconnect_cb)
{
	// [TODO] add null check also check that this connection is still connected
	gattlib_context_t* conn_context = connection->context;
	if (conn_context != NULL)
	{
		conn_context->disconnect_cb = disconnect_cb;
		OrgBluezDevice1* device = conn_context->device;
		// Register a handle for notification
		conn_context->handler_id =
		g_signal_connect(device,
				"g-properties-changed",
				G_CALLBACK (on_handle_device_property_change_connection),
				connection);
		if (conn_context->handler_id < 1)
		{
			return -1;
		}
	}
	return 0;
}

/**
 * @param src		Local Adaptater interface
 * @param dst		Remote Bluetooth address
 * @param dst_type	Set LE address type (either BDADDR_LE_PUBLIC or BDADDR_LE_RANDOM)
 * @param sec_level	Set security level (either BT_IO_SEC_LOW, BT_IO_SEC_MEDIUM, BT_IO_SEC_HIGH)
 * @param psm       Specify the PSM for GATT/ATT over BR/EDR
 * @param mtu       Specify the MTU size
 */
gatt_connection_t *gattlib_connect(const char *src, const char *dst,
				uint8_t dest_type, gattlib_bt_sec_level_t sec_level, int psm, int mtu)
{
	GError *error = NULL;
	const char* adapter_name;
	char device_address_str[20];
	char object_path[100];
	int i;

	if (src) {
		adapter_name = src;
	} else {
		adapter_name = "hci0";
	}
	//printf("[GATTLIB]gattlib_connect\n");
	// Transform string from 'DA:94:40:95:E0:87' to 'dev_DA_94_40_95_E0_87'
	strncpy(device_address_str, dst, sizeof(device_address_str));
	for (i = 0; i < strlen(device_address_str); i++) {
		if (device_address_str[i] == ':') {
			device_address_str[i] = '_';
		}
	}

	// Generate object path like: /org/bluez/hci0/dev_DA_94_40_95_E0_87
	snprintf(object_path, sizeof(object_path), "/org/bluez/%s/dev_%s", adapter_name, device_address_str);

	gattlib_context_t* conn_context = calloc(sizeof(gattlib_context_t), 1);
	if (conn_context == NULL) {
		return NULL;
	}

	gatt_connection_t* connection = calloc(sizeof(gatt_connection_t), 1);
	if (connection == NULL) {
		free(conn_context);
		return NULL;
	} else {
		connection->context = conn_context;
	}

	OrgBluezDevice1* device = org_bluez_device1_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			object_path,
			NULL,
			&error);
	if (device == NULL) {
		goto FREE_CONNECTION;
	} else {
		conn_context->device = device;
		conn_context->device_object_path = strdup(object_path);
		conn_context->disconnect_cb = NULL;
	}

	error = NULL;
	org_bluez_device1_call_connect_sync(device, NULL, &error);
	if (error) {
		//printf("Device connected error: %s\n", error->message);
		goto FREE_DEVICE;
	}

//	// Wait for the property 'UUIDs' to be changed. We assume 'org.bluez.GattService1
//	// and 'org.bluez.GattCharacteristic1' to be advertised at that moment.

	// Now we only wait for one second
	GMainLoop *loop = g_main_loop_new(NULL, 0);
//
//	// Register a handle for notification
//	g_signal_connect(device,
//		"g-properties-changed",
//		G_CALLBACK (on_handle_device_property_change),
//		loop);
//

	g_timeout_add_seconds (CONNECT_TIMEOUT, stop_scan_func, loop);
	g_main_loop_run(loop);
	g_main_loop_unref(loop);

	/*printf("Connection context: 0x%x\n", connection->context);
	printf("Here: %s\n", conn_context->device_object_path);
	printf("RSSI: %d\n", (int)org_bluez_device1_get_rssi(conn_context->device));
	printf("TX Power: %d\n", (int)org_bluez_device1_get_tx_power(conn_context->device));
	printf("Appearance: %d\n", (int)org_bluez_device1_get_appearance(conn_context->device));
	printf("Name: %s\n", org_bluez_device1_get_name(conn_context->device));
	printf("Adapter: %s\n", org_bluez_device1_get_adapter(conn_context->device));
	printf("Connection value: 0x%x\n", connection);*/

	return connection;

FREE_DEVICE:
	free(conn_context->device_object_path);
	g_object_unref(conn_context->device);

FREE_CONNECTION:
	free(conn_context);
	free(connection);
	return NULL;
}

gatt_connection_t *gattlib_connect_async(const char *src, const char *dst,
				uint8_t dest_type, gattlib_bt_sec_level_t sec_level, int psm, int mtu,
				gatt_connect_cb_t connect_cb)
{
	return NULL;
}

int gattlib_rssi(gatt_connection_t* connection)
{
	gattlib_context_t* conn_context = (gattlib_context_t*)connection->context;
	/*printf("Here: %s\n", conn_context->device_object_path);
	printf("RSSI: %d\n", org_bluez_device1_get_rssi(conn_context->device));
	printf("TX Power: %d\n", org_bluez_device1_get_tx_power(conn_context->device));
	printf("Appearance: %u\n", org_bluez_device1_get_appearance(conn_context->device));
	printf("Name: %s\n", org_bluez_device1_get_name(conn_context->device));
	printf("Adapter: %s\n", org_bluez_device1_get_adapter(conn_context->device));*/
	//printf("RSSI: %u\n", org_bluez_device1_get_rssi(device));
	return org_bluez_device1_get_rssi(conn_context->device);
}

int gattlib_disconnect(gatt_connection_t* connection, bool call_disconnect) {
	//printf("disconnect1\n");
	gattlib_context_t* conn_context = connection->context;
	//printf("disconnect2\n");
	GError *error = NULL;
	//printf("[GATTLIB]disconnecting handler Id = %d device = %p\n", conn_context->handler_id, conn_context->device );
	g_signal_handler_disconnect (conn_context->device, conn_context->handler_id);
	if(call_disconnect) org_bluez_device1_call_disconnect_sync(conn_context->device, NULL, &error);
	//printf("disconnect3\n");
	free(conn_context->device_object_path);
	//printf("disconnect4\n");
	g_object_unref(conn_context->device);
	//printf("disconnect5\n");

	free(connection->context);
	//printf("disconnect6\n");
	free(connection);
	//printf("disconnect7\n");
	return 0;
}

// Bluez was using org.bluez.Device1.GattServices until 5.37 to expose the list of available GATT Services
#if BLUEZ_VERSION < BLUEZ_VERSIONS(5, 38)
int gattlib_discover_primary(gatt_connection_t* connection, gattlib_primary_service_t** services, int* services_count) {
	gattlib_context_t* conn_context = connection->context;
	OrgBluezDevice1* device = conn_context->device;
	const gchar* const* service_str;
	GError *error = NULL;

	const gchar* const* service_strs = org_bluez_device1_get_gatt_services(device);

	if (service_strs == NULL) {
		*services       = NULL;
		*services_count = 0;
		return 0;
	}

	// Maximum number of primary services
	int count_max = 0, count = 0;
	for (service_str = service_strs; *service_str != NULL; service_str++) {
		count_max++;
	}

	gattlib_primary_service_t* primary_services = malloc(count_max * sizeof(gattlib_primary_service_t));
	if (primary_services == NULL) {
		return -1;
	}

	for (service_str = service_strs; *service_str != NULL; service_str++) {
		error = NULL;
		OrgBluezGattService1* service_proxy = org_bluez_gatt_service1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				*service_str,
				NULL,
				&error);
		if (service_proxy == NULL) {
			//printf("Failed to open service '%s'.\n", *service_str);
			continue;
		}

		if (org_bluez_gatt_service1_get_primary(service_proxy)) {
			/** Naeem's editing ***/
			gchar *p_uuid = org_bluez_gatt_service1_get_uuid(service_proxy);
			if(!p_uuid)
			{
				g_object_unref(service_proxy);
				service_proxy = NULL;
				continue;
			}
			/**********************/
			primary_services[count].attr_handle_start = 0;
			primary_services[count].attr_handle_end   = 0;

			gattlib_string_to_uuid(
					p_uuid,
					MAX_LEN_UUID_STR + 1,
					&primary_services[count].uuid);
			count++;
		}

		g_object_unref(service_proxy);
		service_proxy=NULL;
	}

	*services       = primary_services;
	*services_count = count;
	return 0;
}
#else
int gattlib_discover_primary(gatt_connection_t* connection, gattlib_primary_service_t** services, int* services_count) {
	gattlib_context_t* conn_context = connection->context;
	OrgBluezDevice1* device = conn_context->device;
	const gchar* const* service_str;
	GError *error = NULL;

	const gchar* const* service_strs = org_bluez_device1_get_uuids(device);

	if (service_strs == NULL) {
		*services       = NULL;
		*services_count = 0;
		return 0;
	}

	// Maximum number of primary services
	int count_max = 0, count = 0;
	for (service_str = service_strs; *service_str != NULL; service_str++) {
		count_max++;
	}

	gattlib_primary_service_t* primary_services = malloc(count_max * sizeof(gattlib_primary_service_t));
	if (primary_services == NULL) {
		return -1;
	}

	GDBusObjectManager *device_manager = g_dbus_object_manager_client_new_for_bus_sync (
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/",
			NULL, NULL, NULL, NULL,
			&error);
	if (device_manager == NULL) {
		puts("Failed to get Bluez Device Manager.");
		return -1;
	}

	GList *objects = g_dbus_object_manager_get_objects(device_manager);
	GList *l;
	for (l = objects; l != NULL; l = l->next)  {
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.GattService1");
		if (!interface) {
			continue;
		}

		error = NULL;
		OrgBluezGattService1* service_proxy = org_bluez_gatt_service1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);
		if (service_proxy == NULL) {
			//printf("Failed to open service '%s'.\n", object_path);
			continue;
		}

		// Ensure the service is attached to this device
		if (strcmp(conn_context->device_object_path, org_bluez_gatt_service1_get_device(service_proxy))) {
			/** Naeem's editing***/
			g_object_unref(service_proxy);
			service_proxy = NULL;
			/********************/
			continue;
		}

		if (org_bluez_gatt_service1_get_primary(service_proxy)) {
			/** Naeem's editing ***/
			gchar *p_uuid = (gchar *) org_bluez_gatt_service1_get_uuid(service_proxy);
			if(!p_uuid)
			{
				g_object_unref(service_proxy);
				service_proxy = NULL;
				continue;
			}
			/**********************/
			primary_services[count].attr_handle_start = 0;
			primary_services[count].attr_handle_end   = 0;

			gattlib_string_to_uuid(
					p_uuid,
					MAX_LEN_UUID_STR + 1,
					&primary_services[count].uuid);
			count++;
			/*** Naeem's editing ***/
			g_object_unref(service_proxy);
			service_proxy =NULL;
			/*********/
		}
	}

	g_list_free_full(objects, g_object_unref);
	g_object_unref(device_manager);

	*services       = primary_services;
	*services_count = count;
	return 0;
}
#endif

int gattlib_discover_char_range(gatt_connection_t* connection, int start, int end, gattlib_characteristic_t** characteristics, int* characteristics_count) {
	return -1;
}

// Bluez was using org.bluez.Device1.GattServices until 5.37 to expose the list of available GATT Services
#if BLUEZ_VERSION < BLUEZ_VERSIONS(5, 38)
int gattlib_discover_char(gatt_connection_t* connection, gattlib_characteristic_t** characteristics, int* characteristic_count) {
	gattlib_context_t* conn_context = connection->context;
	OrgBluezDevice1* device = conn_context->device;
	GError *error = NULL;

	const gchar* const* service_strs = org_bluez_device1_get_gatt_services(device);
	const gchar* const* service_str;
	const gchar* const* characteristic_strs;
	const gchar* const* characteristic_str;

	if (service_strs == NULL) {
		return 2;
	}

	// Maximum number of primary services
	int count_max = 0, count = 0;
	for (service_str = service_strs; *service_str != NULL; service_str++) {
		error = NULL;
		OrgBluezGattService1* service_proxy = org_bluez_gatt_service1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				*service_str,
				NULL,
				&error);
		if (service_proxy == NULL) {
			//printf("Failed to open services '%s'.\n", *service_str);
			continue;
		}

		characteristic_strs = org_bluez_gatt_service1_get_characteristics(service_proxy);
		if (characteristic_strs == NULL) {
			continue;
		}

		for (characteristic_str = characteristic_strs; *characteristic_str != NULL; characteristic_str++) {
			count_max++;
		}
		g_object_unref(service_proxy);
	}


	gattlib_characteristic_t* characteristic_list = malloc(count_max * sizeof(gattlib_characteristic_t));
	if (characteristic_list == NULL) {
		return -1;
	}

	for (service_str = service_strs; *service_str != NULL; service_str++) {
		error = NULL;
		OrgBluezGattService1* service_proxy = org_bluez_gatt_service1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				*service_str,
				NULL,
				&error);
		if (service_proxy == NULL) {
			//printf("Failed to open service '%s'.\n", *service_str);
			continue;
		}

		characteristic_strs = org_bluez_gatt_service1_get_characteristics(service_proxy);
		if (characteristic_strs == NULL) {
			continue;
		}

		for (characteristic_str = characteristic_strs; *characteristic_str != NULL; characteristic_str++) {
			OrgBluezGattCharacteristic1 *characteristic_proxy = org_bluez_gatt_characteristic1_proxy_new_for_bus_sync(
					G_BUS_TYPE_SYSTEM,
					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
					"org.bluez",
					*characteristic_str,
					NULL,
					&error);
			if (characteristic_proxy == NULL) {
				//printf("Failed to open characteristic '%s'.\n", *characteristic_str);
				continue;
			} else {
				characteristic_list[count].handle       = 0;
				characteristic_list[count].value_handle = 0;

				const gchar *const * flags = org_bluez_gatt_characteristic1_get_flags(characteristic_proxy);
				for (; *flags != NULL; flags++) {
					if (strcmp(*flags,"broadcast") == 0) {
						characteristic_list[count].properties |= GATTLIB_CHARACTERISTIC_BROADCAST;
					} else if (strcmp(*flags,"read") == 0) {
						characteristic_list[count].properties |= GATTLIB_CHARACTERISTIC_READ;
					} else if (strcmp(*flags,"write") == 0) {
						characteristic_list[count].properties |= GATTLIB_CHARACTERISTIC_WRITE;
					} else if (strcmp(*flags,"write-without-response") == 0) {
						characteristic_list[count].properties |= GATTLIB_CHARACTERISTIC_WRITE_WITHOUT_RESP;
					} else if (strcmp(*flags,"notify") == 0) {
						characteristic_list[count].properties |= GATTLIB_CHARACTERISTIC_NOTIFY;
					} else if (strcmp(*flags,"indicate") == 0) {
						characteristic_list[count].properties |= GATTLIB_CHARACTERISTIC_INDICATE;
					}
				}

				gattlib_string_to_uuid(
						org_bluez_gatt_characteristic1_get_uuid(characteristic_proxy),
						MAX_LEN_UUID_STR + 1,
						&characteristic_list[count].uuid);
				count++;
			}
			g_object_unref(characteristic_proxy);
		}
		g_object_unref(service_proxy);
	}

	*characteristics      = characteristic_list;
	*characteristic_count = count;
	return 0;
}
#else
static void add_characteristics_from_service(GDBusObjectManager *device_manager, const char* service_object_path, gattlib_characteristic_t* characteristic_list, int* count) {
	GList *objects = g_dbus_object_manager_get_objects(device_manager);
	GError *error = NULL;
	GList *l;

	for (l = objects; l != NULL; l = l->next) {
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));
		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.GattCharacteristic1");
		if (!interface) {
			continue;
		}

		OrgBluezGattCharacteristic1* characteristic = org_bluez_gatt_characteristic1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);
		if (characteristic == NULL) {
			//printf("Failed to open characteristic '%s'.\n", object_path);
			continue;
		}

		if (strcmp(org_bluez_gatt_characteristic1_get_service(characteristic), service_object_path)) {
			g_object_unref(characteristic);
			characteristic =NULL;
			continue;
		} else {
			characteristic_list[*count].handle       = 0;
			characteristic_list[*count].value_handle = 0;

			const gchar *const * flags = org_bluez_gatt_characteristic1_get_flags(characteristic);
			for (; *flags != NULL; flags++) {
				if (strcmp(*flags,"broadcast") == 0) {
					characteristic_list[*count].properties |= GATTLIB_CHARACTERISTIC_BROADCAST;
				} else if (strcmp(*flags,"read") == 0) {
					characteristic_list[*count].properties |= GATTLIB_CHARACTERISTIC_READ;
				} else if (strcmp(*flags,"write") == 0) {
					characteristic_list[*count].properties |= GATTLIB_CHARACTERISTIC_WRITE;
				} else if (strcmp(*flags,"write-without-response") == 0) {
					characteristic_list[*count].properties |= GATTLIB_CHARACTERISTIC_WRITE_WITHOUT_RESP;
				} else if (strcmp(*flags,"notify") == 0) {
					characteristic_list[*count].properties |= GATTLIB_CHARACTERISTIC_NOTIFY;
				} else if (strcmp(*flags,"indicate") == 0) {
					characteristic_list[*count].properties |= GATTLIB_CHARACTERISTIC_INDICATE;
				}
			}
			/** Naeem's editing ***/
			gchar *p_uuid = (gchar *) org_bluez_gatt_characteristic1_get_uuid(characteristic);
			if(!p_uuid)
			{
				g_object_unref(characteristic);
				characteristic =NULL;
				continue;
			}
			/**********************/
			gattlib_string_to_uuid(
					p_uuid,
					MAX_LEN_UUID_STR + 1,
					&characteristic_list[*count].uuid);
			*count = *count + 1;

			g_object_unref(characteristic);
			characteristic =NULL;
		}
	}
	g_list_free_full(objects, g_object_unref);
}

int gattlib_discover_char(gatt_connection_t* connection, gattlib_characteristic_t** characteristics, int* characteristic_count) {
	gattlib_context_t* conn_context = connection->context;
	GError *error = NULL;
	GList *l;

	// Get list of services
	GDBusObjectManager *device_manager = g_dbus_object_manager_client_new_for_bus_sync (
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/",
			NULL, NULL, NULL, NULL,
			&error);
	if (device_manager == NULL) {
		puts("Failed to get Bluez Device Manager.");
		return -1;
	}
	GList *objects = g_dbus_object_manager_get_objects(device_manager);

	// Count the maximum number of characteristic to allocate the array (we count all the characterstic for all devices)
	int count_max = 0, count = 0;
	for (l = objects; l != NULL; l = l->next) {
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));
		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.GattCharacteristic1");
		if (!interface) {
			continue;
		}
		count_max++;
	}

	gattlib_characteristic_t* characteristic_list = malloc(count_max * sizeof(gattlib_characteristic_t));
	if (characteristic_list == NULL) {
		return -1;
	}

	// List all services for this device
	for (l = objects; l != NULL; l = l->next) {
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.GattService1");
		if (!interface) {
			continue;
		}

		error = NULL;
		OrgBluezGattService1* service_proxy = org_bluez_gatt_service1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);
		if (service_proxy == NULL) {
			//printf("Failed to open service '%s'.\n", object_path);
			continue;
		}

		// Ensure the service is attached to this device
		const char* service_object_path = org_bluez_gatt_service1_get_device(service_proxy);
		if (strcmp(conn_context->device_object_path, service_object_path)) {
			continue;
		}

		// Add all characteristics attached to this service
		add_characteristics_from_service(device_manager, object_path, characteristic_list, &count);
	}

	g_list_free_full(objects, g_object_unref);
	g_object_unref(device_manager);

	*characteristics      = characteristic_list;
	*characteristic_count = count;
	return 0;
}
#endif

static OrgBluezGattCharacteristic1 *get_characteristic_from_uuid(const uuid_t* uuid, char* unique_path) {
	OrgBluezGattCharacteristic1 *characteristic = NULL;
	GError *error = NULL;
	////printf("Unique path: %s\n", unique_path);
	GDBusObjectManager *device_manager = g_dbus_object_manager_client_new_for_bus_sync (
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/",
			NULL, NULL, NULL, NULL,
			&error);
	if (device_manager == NULL) {
		puts("Failed to get Bluez Device Manager.");
		//printf("Error: %s\n", error->message);
		return NULL;
	}

	GList *objects = g_dbus_object_manager_get_objects(device_manager);
	GList *l;
	for (l = objects; l != NULL; l = l->next)  {
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		if(strstr(object_path, unique_path) == NULL)
			continue;

		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.GattCharacteristic1");
		if (!interface) {
			continue;
		}
		//printf("Object path: %s\n", object_path);
		error = NULL;
		characteristic = org_bluez_gatt_characteristic1_proxy_new_for_bus_sync (
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);
		if (characteristic) {
			uuid_t characteristic_uuid;
			const gchar *characteristic_uuid_str = org_bluez_gatt_characteristic1_get_uuid(characteristic);
			/** naeem's editing ***/
			if(! characteristic_uuid_str)
			{
				g_object_unref(characteristic);
				characteristic =NULL;
				continue;
			}
			/****/
			gattlib_string_to_uuid(characteristic_uuid_str, strlen(characteristic_uuid_str) + 1, &characteristic_uuid);
			if (gattlib_uuid_cmp(uuid, &characteristic_uuid) == 0) {
				break;
			}

			g_object_unref(characteristic);
			characteristic =NULL;
		}

		// Ensure we set 'characteristic' back to NULL
		characteristic = NULL;
	}

	g_list_free_full(objects, g_object_unref);
	g_object_unref(device_manager);
	return characteristic;
}

int gattlib_discover_desc_range(gatt_connection_t* connection, int start, int end, gattlib_descriptor_t** descriptors, int* descriptor_count) {
	return -1;
}

int gattlib_discover_desc(gatt_connection_t* connection, gattlib_descriptor_t** descriptors, int* descriptor_count) {
	return -1;
}

int gattlib_read_char_by_uuid(gatt_connection_t* connection, uuid_t* uuid, void* buffer, size_t* buffer_len) {
	/*printf("Here2: 0x%x\n", connection);
	printf("Connection context: 0x%x\n", connection->context);*/
	gattlib_context_t* test =  (gattlib_context_t*)connection->context;
	//printf("Here3\n");
	OrgBluezGattCharacteristic1 *characteristic = get_characteristic_from_uuid(uuid, test->device_object_path);
	if (characteristic == NULL) {
		return -1;
	}

	GVariant *out_value;
	GError *error = NULL;

	//printf("bluez version %d.%d\n",BLUEZ_VERSION_MAJOR, BLUEZ_VERSION_MINOR);

#if BLUEZ_VERSION < BLUEZ_VERSIONS(5, 40)
	org_bluez_gatt_characteristic1_call_read_value_sync(
		characteristic, &out_value, NULL, &error);
#else

	GVariantBuilder *options =  g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	org_bluez_gatt_characteristic1_call_read_value_sync(
		characteristic, g_variant_builder_end(options), &out_value, NULL, &error);
	g_variant_builder_unref(options);
#endif
	if (error != NULL) {
		/** Naeem's editing ***/
		g_object_unref(characteristic);
		characteristic =NULL;
		return -1;
	}

	gsize n_elements = 0;
	gconstpointer const_buffer = g_variant_get_fixed_array(out_value, &n_elements, sizeof(guchar));
	if (const_buffer) {
		n_elements = MIN(n_elements, *buffer_len);
		memcpy(buffer, const_buffer, n_elements);
	}

	*buffer_len = n_elements;

	g_object_unref(characteristic);
	characteristic = NULL;

#if BLUEZ_VERSION >= BLUEZ_VERSIONS(5, 40)
	//g_variant_unref(in_params); See: https://github.com/labapart/gattlib/issues/28#issuecomment-311486629
#endif
	return 0;
}

int gattlib_read_char_by_uuid_async(gatt_connection_t* connection, uuid_t* uuid, gatt_read_cb_t gatt_read_cb) {
	gattlib_context_t* test =  (gattlib_context_t*)connection->context;
	OrgBluezGattCharacteristic1 *characteristic = get_characteristic_from_uuid(uuid, test->device_object_path);
	if (characteristic == NULL) {
		return -1;
	}

	GVariant *out_value;
	GError *error = NULL;

#if BLUEZ_VERSION < BLUEZ_VERSIONS(5, 40)
	org_bluez_gatt_characteristic1_call_read_value_sync(
		characteristic, &out_value, NULL, &error);
#else
	GVariantBuilder *options =  g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	org_bluez_gatt_characteristic1_call_read_value_sync(
		characteristic, g_variant_builder_end(options), &out_value, NULL, &error);
	g_variant_builder_unref(options);
#endif
	if (error != NULL) {
		return -1;
	}

	gsize n_elements;
	gconstpointer const_buffer = g_variant_get_fixed_array(out_value, &n_elements, sizeof(guchar));
	if (const_buffer) {
		gatt_read_cb(const_buffer, n_elements);
	}

	g_object_unref(characteristic);

#if BLUEZ_VERSION >= BLUEZ_VERSIONS(5, 40)
	//g_variant_unref(in_params); See: https://github.com/labapart/gattlib/issues/28#issuecomment-311486629
#endif
	return 0;
}

int gattlib_write_char_by_uuid(gatt_connection_t* connection, uuid_t* uuid, const void* buffer, size_t buffer_len) {
	gattlib_context_t* test =  (gattlib_context_t*)connection->context;
	OrgBluezGattCharacteristic1 *characteristic = get_characteristic_from_uuid(uuid, test->device_object_path);
	if (characteristic == NULL) {
		//printf("Characteristic is null\n");
		return -1;
	}
	//printf("Characteristic is not null\n");
	GVariant *value = g_variant_new_from_data(G_VARIANT_TYPE ("ay"), buffer, buffer_len, TRUE, NULL, NULL);
	GError *error = NULL;

#if BLUEZ_VERSION < BLUEZ_VERSIONS(5, 40)
	org_bluez_gatt_characteristic1_call_write_value_sync(characteristic, value, NULL, &error);
#else
	GVariantBuilder *options =  g_variant_builder_new(G_VARIANT_TYPE("a{sv}"));
	org_bluez_gatt_characteristic1_call_write_value_sync(characteristic, value, g_variant_builder_end(options), NULL, &error);
	g_variant_builder_unref(options);
#endif
	if (error != NULL) {
		//printf ("Unable to read file: %s\n", error->message);
		return -1;
	}

	g_object_unref(characteristic);
#if BLUEZ_VERSION >= BLUEZ_VERSIONS(5, 40)
	//g_variant_unref(in_params); See: https://github.com/labapart/gattlib/issues/28#issuecomment-311486629
#endif
	return 0;
}

int gattlib_write_char_by_handle(gatt_connection_t* connection, uint16_t handle, const void* buffer, size_t buffer_len) {
	return -1;
}

gboolean on_handle_characteristic_property_change(
	    OrgBluezGattCharacteristic1 *object,
	    GVariant *arg_changed_properties,
	    const gchar *const *arg_invalidated_properties,
	    gpointer user_data)
{
	gatt_connection_t* connection = user_data;

	if (connection->notification_handler) {
		// Retrieve 'Value' from 'arg_changed_properties'
		if (g_variant_n_children (arg_changed_properties) > 0) {
			GVariantIter *iter;
			const gchar *key;
			GVariant *value;

			g_variant_get (arg_changed_properties, "a{sv}", &iter);
			while (g_variant_iter_loop (iter, "{&sv}", &key, &value)) {
				if (strcmp(key, "Value") == 0) {
					uuid_t uuid;
					size_t data_length;
					const uint8_t* data = g_variant_get_fixed_array(value, &data_length, sizeof(guchar));
					/** Naeem's editing ***/
					gchar *p_uuid =(gchar *) org_bluez_gatt_characteristic1_get_uuid(object);
					if(!p_uuid)
					{
						//printf("\033[0;31m \nERROR: Data missed at gattlib due to error\n");
						//printf("\033[0m");
						continue;
					}
					/**********************/
					gattlib_string_to_uuid(
							p_uuid,
							MAX_LEN_UUID_STR + 1,
							&uuid);

					connection->notification_handler(&uuid, data, data_length, connection->notification_user_data);
					break;
				}
			}
		}
	}
	return TRUE;
}

/* Given a reference (pointer to pointer) to the head
   of a list and an int, appends a new node at the end  */
void append(signal_handler_t** head_ref, gulong handler_id, const uuid_t *uuid, OrgBluezGattCharacteristic1 *characteristic)
{
    /* 1. allocate node */
    signal_handler_t* new_node = (signal_handler_t*) malloc(sizeof(signal_handler_t));

    signal_handler_t *last = *head_ref;  /* used in step 5*/

    /* 2. put in the data  */
    new_node->handler_id  = handler_id;
    memcpy(&new_node->uuid, uuid, sizeof(uuid_t));
    new_node->characteristic = characteristic;


    /* 3. This new node is going to be the last node, so make next
          of it as NULL*/
    new_node->next = NULL;

    /* 4. If the Linked List is empty, then make the new node as head */
    if (*head_ref == NULL)
    {
       *head_ref = new_node;
       return;
    }

    /* 5. Else traverse till the last node */
    while (last->next != NULL)
        last = last->next;

    /* 6. Change the next of last node */
    last->next = new_node;
    return;
}

/* Given a reference (pointer to pointer) to the head of a list
   and a key, deletes the first occurrence of key in linked list */
signal_handler_t* deleteNode(signal_handler_t **head_ref, const uuid_t *uuid)
{
    // Store head node
	signal_handler_t* temp = *head_ref, *prev;

    // If head node itself holds the key to be deleted
    if (temp != NULL && gattlib_uuid_cmp(&temp->uuid, uuid) == 0)
    {
        *head_ref = temp->next;   // Changed head
        //free(temp);               // free old head
        return temp;
    }

    // Search for the key to be deleted, keep track of the
    // previous node as we need to change 'prev->next'
    while (temp != NULL && gattlib_uuid_cmp(&temp->uuid, uuid) != 0)
    {
        prev = temp;
        temp = temp->next;
    }

    // If key was not present in linked list
    if (temp == NULL) return NULL;

    // Unlink the node from linked list
    prev->next = temp->next;

    //free(temp);  // Free memory
    return temp;
}

int gattlib_notification_start(gatt_connection_t* connection, const uuid_t* uuid) {
	gattlib_context_t* test =  (gattlib_context_t*)connection->context;
	OrgBluezGattCharacteristic1 *characteristic = get_characteristic_from_uuid(uuid, test->device_object_path);
	if (characteristic == NULL) {
		return -1;
	}

	// Register a handle for notification
	gulong handler_id =
	g_signal_connect(characteristic,
		"g-properties-changed",
		G_CALLBACK (on_handle_characteristic_property_change),
		connection);

	//printf("[GATTLIB]Connected_handler =: %d %p \n", handler_id, characteristic);
	GError *error = NULL;
	org_bluez_gatt_characteristic1_call_start_notify_sync(characteristic, NULL, &error);

	if (error) {
		//puts("Failed to start gattlib_notification_start.");
		printf("Error: %s\n", error->message);
		if (handler_id > 0) {
			g_signal_handler_disconnect(characteristic, handler_id);
		}
		return -1;
	}  if (handler_id <= 0) {
                puts("Failed to start gattlib_notification_start due to invalid handler.");
                return -1;
    } else {
		append(&test->notification_handlers, handler_id, uuid, characteristic );
		return 0;
	}
}

int gattlib_notification_stop(gatt_connection_t* connection, const uuid_t* uuid) {
	gattlib_context_t* test =  (gattlib_context_t*)connection->context;
	OrgBluezGattCharacteristic1 *characteristic = get_characteristic_from_uuid(uuid, test->device_object_path);
	if (characteristic == NULL) {
		return -1;
	}

	GError *error = NULL;
	org_bluez_gatt_characteristic1_call_stop_notify_sync(
		characteristic, NULL, &error);

	signal_handler_t *search_node = deleteNode(&test->notification_handlers,  uuid);
	if (search_node != NULL) {
		//printf("[GATTLIB]Disconnected_handler =: %d %p \n", search_node->handler_id, search_node->characteristic);
		g_signal_handler_disconnect(search_node->characteristic, search_node->handler_id);
		free(search_node);
	}
	if (error) {
		printf("Error: %s\n", error->message);
		return -1;
	} else {
		return 0;
	}
}

static int dev_paired_stat(OrgBluezDevice1 *object)
{
	return org_bluez_device1_get_paired(object) ? SECURE : UNSECURE;
}

int gattlib_pair(const char *adapter ,const char *address)
{
	GError *error = NULL;
	const char* adapter_name;
	char device_address_str[20];
	char object_path[100];
	int i,ret=0;

	if (adapter) {
		adapter_name = adapter;
	}
	else
	{
		adapter_name = "hci0";
	}
	// Transform string from 'DA:94:40:95:E0:87' to 'dev_DA_94_40_95_E0_87'
	strncpy(device_address_str, address, sizeof(device_address_str));
	for (i = 0; i < strlen(device_address_str); i++) {
		if (device_address_str[i] == ':') {
			device_address_str[i] = '_';
		}
	}

	// Generate object path like: /org/bluez/hci0/dev_DA_94_40_95_E0_87
	snprintf(object_path, sizeof(object_path), "/org/bluez/%s/dev_%s", adapter_name, device_address_str);
	//printf("object path %s\n",object_path);
	OrgBluezDevice1* device = org_bluez_device1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);
	if (device == NULL)
	{
		printf("Device object creation error: %s\n", error->message);
		return -1;
	}
	else
	{
		error = NULL;
		// if already paired, don't try as it will return error
		if(! dev_paired_stat(device))
		{
			if( org_bluez_device1_call_pair_sync(device,NULL,&error) )
			{
				ret = 0;
			}else
			{
				printf("Device pairing error: %s\n", error->message);
				ret = -1;
			}
		}
		else
		{
			//printf("device [%s] is already paired",address);
			ret = 0;
		}
	}
	g_object_unref(device);
	return ret;
}

int gattlib_is_conn_dev_paired(gatt_connection_t *connection)
{
	gattlib_context_t* conn_context = (gattlib_context_t *)connection->context;
	OrgBluezDevice1* device = conn_context->device;
	return org_bluez_device1_get_paired(device) ? SECURE : UNSECURE;
}

int gattlib_remove_device(const char *adapter, const char * address)
{
	GError *error = NULL;
	const char* adapter_name;
	char device_address_str[20];
	char dev_object_path[100];
	char adapter_object_path[100];
	int i,ret=0;

	if (adapter) {
		adapter_name = adapter;
	}
	else
	{
		adapter_name = "hci0";
	}
	// Transform string from 'DA:94:40:95:E0:87' to 'dev_DA_94_40_95_E0_87'
	strncpy(device_address_str, address, sizeof(device_address_str));
	for (i = 0; i < strlen(device_address_str); i++) {
		if (device_address_str[i] == ':') {
			device_address_str[i] = '_';
		}
	}

	// Generate object path like: /org/bluez/hci0/dev_DA_94_40_95_E0_87
	snprintf(dev_object_path, sizeof(dev_object_path), "/org/bluez/%s/dev_%s", adapter_name, device_address_str);
	snprintf(adapter_object_path, sizeof(adapter_object_path), "/org/bluez/%s", adapter_name);
	//printf("object path %s\n",dev_object_path);
	OrgBluezAdapter1 *device = org_bluez_adapter1_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			adapter_object_path,
			NULL,
			&error);
	if(! device)
	{
		//printf("getting adapter error: %s\n", error->message);
		return -1;
	}

	if(org_bluez_adapter1_call_remove_device_sync (device,dev_object_path,NULL,&error) )
	{
		//printf("removed \n");
		ret = 0;
	}
	else
	{
		//printf("removing device error: %s\n", error->message);
		ret = -1;
	}
	g_object_unref(device);
	return ret;
}

int gattlib_disconnect_using_address(const char *adapter, const char * address)
{
	GError *error = NULL;
	const char* adapter_name;
	char device_address_str[20];
	char dev_object_path[100];
	int i,ret=0;

	if (adapter) {
		adapter_name = adapter;
	}
	else
	{
		adapter_name = "hci0";
	}
	// Transform string from 'DA:94:40:95:E0:87' to 'dev_DA_94_40_95_E0_87'
	strncpy(device_address_str, address, sizeof(device_address_str));
	for (i = 0; i < strlen(device_address_str); i++) {
		if (device_address_str[i] == ':') {
			device_address_str[i] = '_';
		}
	}

	// Generate object path like: /org/bluez/hci0/dev_DA_94_40_95_E0_87
	snprintf(dev_object_path, sizeof(dev_object_path), "/org/bluez/%s/dev_%s", adapter_name, device_address_str);
	//printf("object path %s\n",dev_object_path);
	OrgBluezDevice1* device = org_bluez_device1_proxy_new_for_bus_sync(
						G_BUS_TYPE_SYSTEM,
						G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
						"org.bluez",
						dev_object_path,
						NULL,
						&error);
	if(! device)
	{
		//printf("getting adapter error: %s\n", error->message);
		return -1;
	}

	if(org_bluez_device1_call_disconnect_sync(device, NULL, &error) )
	{
		//printf("Disconnected \n");
		ret = 0;
	}
	else
	{
		//printf("Disconnection error: %s\n", error->message);
		ret = -1;
	}
	g_object_unref(device);
	return ret;
}

int gattlib_get_connected_devices(gattlib_connected_device_t connected_device_cb)
{
	GDBusObjectManager *device_manager;
	GError *error = NULL;

	/*get manager obj for bluez*/
	device_manager = g_dbus_object_manager_client_new_for_bus_sync (
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/",
			NULL, NULL, NULL, NULL,
			&error);
	if (device_manager == NULL) {
		//printf("Failed to get Bluez Device Manager.");
		return -1;
	}
    /*get all objects i.e. devices objs*/
	GList *objects = g_dbus_object_manager_get_objects(device_manager);
	GList *l;
	for (l = objects; l != NULL; l = l->next)
	{
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.Device1");
		if (!interface) {
			continue;
		}

		error = NULL;
		OrgBluezDevice1* device1 = org_bluez_device1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);

		if (device1) {
			if(org_bluez_device1_get_connected(device1))
			{
				connected_device_cb(
					org_bluez_device1_get_address(device1),
					org_bluez_device1_get_name(device1));
			}
			g_object_unref(device1);
		}
	}

	g_list_free_full(objects, g_object_unref);

	g_object_unref(device_manager);
	return 0;
}

int gattlib_get_paired_devices(gattlib_paired_device_t paired_device_cb)
{
	GDBusObjectManager *device_manager;
	GError *error = NULL;

	/*get manager obj for bluez*/
	device_manager = g_dbus_object_manager_client_new_for_bus_sync (
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/",
			NULL, NULL, NULL, NULL,
			&error);
	if (device_manager == NULL) {
		//printf("Failed to get Bluez Device Manager.");
		return -1;
	}
    /*get all objects i.e. devices objs*/
	GList *objects = g_dbus_object_manager_get_objects(device_manager);
	GList *l;
	for (l = objects; l != NULL; l = l->next)
	{
		GDBusObject *object = l->data;
		const char* object_path = g_dbus_object_get_object_path(G_DBUS_OBJECT(object));

		GDBusInterface *interface = g_dbus_object_manager_get_interface(device_manager, object_path, "org.bluez.Device1");
		if (!interface) {
			continue;
		}

		error = NULL;
		OrgBluezDevice1* device1 = org_bluez_device1_proxy_new_for_bus_sync(
				G_BUS_TYPE_SYSTEM,
				G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				"org.bluez",
				object_path,
				NULL,
				&error);

		if (device1) {
			if(org_bluez_device1_get_paired(device1))
			{
				paired_device_cb(
					org_bluez_device1_get_address(device1),
					org_bluez_device1_get_name(device1));
			}
			g_object_unref(device1);
		}
	}

	g_list_free_full(objects, g_object_unref);

	g_object_unref(device_manager);
	return 0;
}

int is_dev_connected(char * adapter,char *address)
{
	GError *error = NULL;
	const char* adapter_name;
	char device_address_str[20];
	char dev_object_path[100];
	int i,ret=0;

	if (adapter) {
		adapter_name = adapter;
	}
	else
	{
		adapter_name = "hci0";
	}
	// Transform string from 'DA:94:40:95:E0:87' to 'dev_DA_94_40_95_E0_87'
	strncpy(device_address_str, address, sizeof(device_address_str));
	for (i = 0; i < strlen(device_address_str); i++) {
		if (device_address_str[i] == ':') {
			device_address_str[i] = '_';
		}
	}

	// Generate object path like: /org/bluez/hci0/dev_DA_94_40_95_E0_87
	snprintf(dev_object_path, sizeof(dev_object_path), "/org/bluez/%s/dev_%s", adapter_name, device_address_str);
	//printf("object path %s\n",dev_object_path);
	OrgBluezDevice1* device = org_bluez_device1_proxy_new_for_bus_sync(
					G_BUS_TYPE_SYSTEM,
					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
					"org.bluez",
					dev_object_path,
					NULL,
					&error);
	if(! device)
	{
		//printf("getting adapter error: %s\n", error->message);
		return -1;
	}

	if(org_bluez_device1_get_connected(device) )
	{
		//printf("Disconnected \n");
		ret = 0;
	}
	else
	{
		ret = -1;
	}
	g_object_unref(device);
	return ret;
}

int is_dev_paired(char * adapter,char *address)
{
	GError *error = NULL;
	const char* adapter_name;
	char device_address_str[20];
	char dev_object_path[100];
	int i,ret=0;

	if (adapter) {
		adapter_name = adapter;
	}
	else
	{
		adapter_name = "hci0";
	}
	// Transform string from 'DA:94:40:95:E0:87' to 'dev_DA_94_40_95_E0_87'
	strncpy(device_address_str, address, sizeof(device_address_str));
	for (i = 0; i < strlen(device_address_str); i++) {
		if (device_address_str[i] == ':') {
			device_address_str[i] = '_';
		}
	}

	// Generate object path like: /org/bluez/hci0/dev_DA_94_40_95_E0_87
	snprintf(dev_object_path, sizeof(dev_object_path), "/org/bluez/%s/dev_%s", adapter_name, device_address_str);
	//printf("object path %s\n",dev_object_path);
	OrgBluezDevice1* device = org_bluez_device1_proxy_new_for_bus_sync(
					G_BUS_TYPE_SYSTEM,
					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
					"org.bluez",
					dev_object_path,
					NULL,
					&error);
	if(! device)
	{
		//printf("getting device error: %s\n", error->message);
		return -1;
	}

	if(org_bluez_device1_get_paired(device) )
	{
		ret = SECURE;
	}
	else
	{
		ret = UNSECURE;
	}
	g_object_unref(device);
	return ret;
}
/*int register_agent(char *agent_path)
{
	GError *error = NULL;
	OrgBluezAgentManager1 *manager = org_bluez_agent_manager1_proxy_new_for_bus_sync(
			G_BUS_TYPE_SYSTEM,
			G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
			"org.bluez",
			"/org/bluez",
			NULL,
			&error);
	if(manager == NULL)
	{
		//printf("agent manager error: %s\n", error->message);
		return -1;
	}else
	{
		error = NULL;
		if(org_bluez_agent_manager1_call_register_agent_sync (
				manager,
				agent_path,
			   "DisplayYesNo",
			   NULL,
			&error) )
		{
			printf("agent registered\n");
			if ( org_bluez_agent_manager1_call_request_default_agent_sync(
					manager,
					agent_path,
					NULL,
					&error
					) )
			{
				printf("default agent registered\n");
				//			OrgBluezAgent1 *agent = org_bluez_agent1_proxy_new_for_bus_sync(
				//					G_BUS_TYPE_SYSTEM,
				//					G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
				//					"org.bluez",
				//					"/org/bluez",
				//					NULL,
				//					&error);
				//			if(agent != NULL)
				//			{
				//				printf("got agent\n");
				//				if(org_bluez_agent1_call_request_confirmation_sync(agent,"yes",1,NULL,&error))
				//				{
				//					printf("confirmed and paired\n");
				//				}else
				//					printf("request confirm  error: %s\n", error->message);
				//			}else
				//			{
				//				printf("agent get error: %s\n", error->message);
				//				return -1;
				//			}
			}
			else
				printf("default agent error: %s\n", error->message);
			return 0;
		}else
		{
			printf("register agent error: %s\n", error->message);
			return -1;
		}
	}
	return 0;
}
*/
