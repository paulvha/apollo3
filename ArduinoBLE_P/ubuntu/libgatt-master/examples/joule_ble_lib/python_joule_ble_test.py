from ctypes import *
import struct
from gi.repository import GLib, GObject
'''class c_u128(Structure):
    _fields_ = [("_0", c_ulonglong),
                 ("_1", c_ulonglong)]'''

class c_u128(Structure):
    _fields_ = [("val", c_ulonglong*2)]
                 

class valuetype(Union):
    _fields_ = [("uuid16", c_ushort),
                 ("uuid32", c_uint),
                 ("uuid128", c_u128)]
     
class uuid_t(Structure):
    _fields_ = [("type", c_ubyte),
                 ("value", valuetype)]
    
def notification_handle(uuid, data, data_length, user_data):
    print 'Notification handler'
    print uuid[0].type
    print '%x %x' %( uuid[0].value.uuid128.val[0], uuid[0].value.uuid128.val[1])
    print data_length
    print data[0]
    '''for i in data:
        print '%x' %(i)'''


    
callback_type = CFUNCTYPE(None, POINTER(uuid_t), POINTER(c_ubyte), c_size_t, c_void_p)
handler = callback_type(notification_handle)
alert_uuid = '45649931-5eb5-4b98-8792-fd9e1c724bb9'
g_uuid=uuid_t()

print callback_type
gattlib = cdll.LoadLibrary('../lib/libgattlib.so')
MAC="00:0B:57:29:FC:F9"

if gattlib.gattlib_string_to_uuid(alert_uuid, len(alert_uuid)+1, byref(g_uuid)) == 0:
    print '%x %x' %( g_uuid.value.uuid128.val[0], g_uuid.value.uuid128.val[1])
    print '%x' %(g_uuid.type)
    uuid_str = create_string_buffer(37)
    if gattlib.gattlib_uuid_to_string(byref(g_uuid), uuid_str, 37) == 0:
        print uuid_str.value
        if alert_uuid == uuid_str.value:
            print 'Success'
    else:
        print 'Error'
connection = gattlib.gattlib_connect(None, MAC, 0x01, 1, 0, 0)

gattlib.gattlib_register_notification(connection, handler, None)
if gattlib.gattlib_notification_start(connection, byref(g_uuid)) == 0:
    print 'Notification started'
else:
    print 'Notification failed'
    
GObject.MainLoop().run()
print 'Here'
exit()   
#joulelib = cdll.LoadLibrary('libjoule_ble_dbus.so')
gattlib = cdll.LoadLibrary('../lib/libgattlib.so')
MAC="00:0B:57:29:FC:F9"
#connection = gattlib.gattlib_connect(None, MAC, 0x01, 1, 0, 0)
brightness_uuid='34596dce-6442-4903-a78e-81635ca92f00'
g_uuid=uuid_t()
if gattlib.gattlib_string_to_uuid(brightness_uuid, len(brightness_uuid)+1, byref(g_uuid)) == 0:
    print '%x %x' %( g_uuid.value.uuid128._1, g_uuid.value.uuid128._0)
connection = gattlib.gattlib_connect(None, MAC, 0x01, 1, 0, 0)
'''write_data=0xABCD
write_buffer=struct.pack('H', write_data)

if gattlib.gattlib_write_char_by_uuid(connection, byref(g_uuid), write_buffer, c_int(2))== 0:
    print 'data written' '''
    
read_buffer=create_string_buffer(256)

read_len=c_int(256)
if gattlib.gattlib_read_char_by_uuid(connection, byref(g_uuid), read_buffer, byref(read_len)) == 0:
    print 'Length of data received', read_len.value
    buffer_copy=read_buffer[:read_len.value]
    print 'Data received: 0x%X' %(struct.unpack('H',buffer_copy))
    print connection
print gattlib.gattlib_disconnect(connection)
exit()


MAC="00:0B:57:29:FC:F9"
connection = gattlib.gattlib_connect(None, MAC, 0x01, 1, 0, 0)
uuid='34596dce-6442-4903-a78e-81635ca92f00'
write_data=0xABCD
write_len=c_int(2)
write_buffer=struct.pack('H', write_data)
joulelib.write_characteristic_by_uuid(connection, uuid, write_buffer, write_len)

buffer=create_string_buffer(256)

len=c_int(256)
print len
while True:
    import time
    time.sleep(60)
    joulelib.read_characteristic_by_uuid(connection, uuid, buffer, byref(len))
    print 'Length of data received', len.value
    buffer_copy=buffer[:len.value]
    print 'Data received: 0x%X' %(struct.unpack('H',buffer_copy))
    print connection
    print gattlib.gattlib_disconnect(connection)
#, buffer, len