/* mbed_ble.h
 *
 * version 1.0 / december 2021 / paulvha
 */

#include "ble/BLE.h"
#include "platform/Callback.h"
#include "platform/NonCopyable.h"
#include "Gap.h"
#include "ble/FunctionPointerWithContext.h"
#include "ble/gap/AdvertisingDataParser.h"
#include "wsf_types.h"
#include "BLETypes.h"
#include "ble/gap/Types.h"
#include "util/bda.h"
#include "DiscoveredCharacteristic.h"
#include "DiscoveredService.h"
#include "ble/GattClient.h"
#include "ble_debug.h"
#include "pretty_printer.h"


// for use of ms
using namespace std::chrono;
using std::milli;
using namespace std::literals::chrono_literals;
