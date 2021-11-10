/** @file
 * Header file for USB JoyStick Data Structure
 *  USB JoyStick Driver that manages USB JoyStick 
 *   YIZD 2021
 *   */


#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_


#include <Uefi.h>

#include<Protocol/SimpleTextIn.h>
#include<Protocol/SimpleTextInEx.h>
#include<Protocol/HiiDatabase.h>
#include<Protocol/UsbIo.h>
#include<Protocol/DevicePath.h>

#include<Library/DebugLib.h>
#include<Library/ReportStatusCodeLib.h>
#include<Library/BaseMemoryLib.h>
#include<Library/UefiRuntimeServicesTableLib.h>
#include<Library/UefiDriverEntryPoint.h>
#include<Library/UefiBootServicesTableLib.h>
#include<Library/UefiLib.h>
#include<Library/MemoryAllocationLib.h>
#include<Library/PcdLib.h>
#include<Library/UefiUsbLib.h>
#include<Library/HiiLib.h>

#include<IndustryStandard/Usb.h>


#define NINTENDO_HID  0x057E
#define JOYSTICK_PID  0x2009

#define USB_JS_DEV_SIGNATURE SIGNATURE_32 ('u', 'k', 'b', 'd')
#define USB_JS_CONSOLE_IN_EX_NOTIFY_SIGNATURE SIGNAGURE_32 ('u', 'k', 'b', 'x')

/*
 * Structure to describe USB JoyStick device
 *
 */

typedef struct{
	UINTN                           Signature;
	EFI_HANDLE                      ControllerHandle;
  EFI_DEVICE_PATH_PROTOCOL        *DevicePath;
	//EFI_EVENT                       DelayedRecoveryEvent;
	EFI_SIMPLE_TEXT_INPUT_PROTOCOL  SimpleInput;
	EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL SimpleInputEx;
	EFI_USB_IO_PROTOCOL             *UsbIo;

	EFI_USB_INTERFACE_DESCRIPTOR    InterfaceDescriptor;
	EFI_USB_ENDPOINT_DESCRIPTOR     IntInEndpointDescriptor;
  EFI_USB_ENDPOINT_DESCRIPTOR     IntOutEndpointDescriptor;
	EFI_UNICODE_STRING_TABLE        *ControllerNameTable;
  
  UINT8                           LastReport[64];
  //UINT8                           CurrReport[64];
}USB_JS_DEV;

typedef struct{
  BOOLEAN   DPAD_DOWN;
  BOOLEAN   DPAD_RIGHT;
  BOOLEAN   DPAD_LEFT;
  BOOLEAN   DPAD_UP;
  BOOLEAN   MINUS;
  BOOLEAN   HOME;
  BOOLEAN   PLUS;
  BOOLEAN   CAPTURE;
  BOOLEAN   STICK;
  BOOLEAN   SHOULDER_1;
  BOOLEAN   SHOULDER_2;
  BOOLEAN   B;
  BOOLEAN   A;
  BOOLEAN   Y;
  BOOLEAN   X;
  BOOLEAN   STICK2;
  BOOLEAN   SHOULDER2_1;
  BOOLEAN   SHOULDER2_2;
}ButtonMap;



extern EFI_DRIVER_BINDING_PROTOCOL   gUsbJoyStickDriverBinding;
extern EFI_COMPONENT_NAME_PROTOCOL   gUsbJoyStickComponentName;
extern EFI_COMPONENT_NAME2_PROTOCOL  gUsbJoyStickComponentName2;

#define USB_JS_DEV_FROM_THIS(a) \
	CR(a,USB_JS_DEV,SimpleInput,USB_JS_DEV_SIGNATURE)
#define TEXT_INPUT_EX_USB_JS_DEV_FROM_THIS(a) \
	CR(a,USB_JS_DEV,SimpleInputEx,USB_JS_DEV_SIGNATURE)






//
// Functions of Driver Binding Protocol
//
/**
  Check whether USB keyboard driver supports this device.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller handle to check.
  @param  RemainingDevicePath    The remaining device path.

  @retval EFI_SUCCESS            The driver supports this controller.
  @retval other                  This device isn't supported.

**/
EFI_STATUS
EFIAPI
USBJoyStickDriverBindingSupported (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Starts the keyboard device with this driver.

  This function produces Simple Text Input Protocol and Simple Text Input Ex Protocol,
  initializes the keyboard device, and submit Asynchronous Interrupt Transfer to manage
  this keyboard device.

  @param  This                   The USB keyboard driver binding instance.
  @param  Controller             Handle of device to bind driver to.
  @param  RemainingDevicePath    Optional parameter use to pick a specific child
                                 device to start.

  @retval EFI_SUCCESS            The controller is controlled by the usb keyboard driver.
  @retval EFI_UNSUPPORTED        No interrupt endpoint can be found.
  @retval Other                  This controller cannot be started.

**/
EFI_STATUS
EFIAPI
USBJoyStickDriverBindingStart (
  IN EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN EFI_HANDLE                     Controller,
  IN EFI_DEVICE_PATH_PROTOCOL       *RemainingDevicePath
  );

/**
  Stop the USB keyboard device handled by this driver.

  @param  This                   The USB keyboard driver binding protocol.
  @param  Controller             The controller to release.
  @param  NumberOfChildren       The number of handles in ChildHandleBuffer.
  @param  ChildHandleBuffer      The array of child handle.

  @retval EFI_SUCCESS            The device was stopped.
  @retval EFI_UNSUPPORTED        Simple Text In Protocol or Simple Text In Ex Protocol
                                 is not installed on Controller.
  @retval EFI_DEVICE_ERROR       The device could not be stopped due to a device error.
  @retval Others                 Fail to uninstall protocols attached on the device.

**/
EFI_STATUS
EFIAPI
USBJoyStickDriverBindingStop (
  IN  EFI_DRIVER_BINDING_PROTOCOL    *This,
  IN  EFI_HANDLE                     Controller,
  IN  UINTN                          NumberOfChildren,
  IN  EFI_HANDLE                     *ChildHandleBuffer
  );

//
// EFI Component Name Functions
//
/**
  Retrieves a Unicode string that is the user readable name of the driver.

  This function retrieves the user readable name of a driver in the form of a
  Unicode string. If the driver specified by This has a user readable name in
  the language specified by Language, then a pointer to the driver name is
  returned in DriverName, and EFI_SUCCESS is returned. If the driver specified
  by This does not support the language specified by Language,
  then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language. This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified
                                in RFC 4646 or ISO 639-2 language code format.
  @param  DriverName            A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                driver specified by This in the language
                                specified by Language.

  @retval EFI_SUCCESS           The Unicode string for the Driver specified by
                                This and the language specified by Language was
                                returned in DriverName.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER DriverName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbJoyStickComponentNameGetDriverName (
  IN  EFI_COMPONENT_NAME_PROTOCOL  *This,
  IN  CHAR8                        *Language,
  OUT CHAR16                       **DriverName
  );

/**
  Retrieves a Unicode string that is the user readable name of the controller
  that is being managed by a driver.

  This function retrieves the user readable name of the controller specified by
  ControllerHandle and ChildHandle in the form of a Unicode string. If the
  driver specified by This has a user readable name in the language specified by
  Language, then a pointer to the controller name is returned in ControllerName,
  and EFI_SUCCESS is returned.  If the driver specified by This is not currently
  managing the controller specified by ControllerHandle and ChildHandle,
  then EFI_UNSUPPORTED is returned.  If the driver specified by This does not
  support the language specified by Language, then EFI_UNSUPPORTED is returned.

  @param  This                  A pointer to the EFI_COMPONENT_NAME2_PROTOCOL or
                                EFI_COMPONENT_NAME_PROTOCOL instance.
  @param  ControllerHandle      The handle of a controller that the driver
                                specified by This is managing.  This handle
                                specifies the controller whose name is to be
                                returned.
  @param  ChildHandle           The handle of the child controller to retrieve
                                the name of.  This is an optional parameter that
                                may be NULL.  It will be NULL for device
                                drivers.  It will also be NULL for a bus drivers
                                that wish to retrieve the name of the bus
                                controller.  It will not be NULL for a bus
                                driver that wishes to retrieve the name of a
                                child controller.
  @param  Language              A pointer to a Null-terminated ASCII string
                                array indicating the language.  This is the
                                language of the driver name that the caller is
                                requesting, and it must match one of the
                                languages specified in SupportedLanguages. The
                                number of languages supported by a driver is up
                                to the driver writer. Language is specified in
                                RFC 4646 or ISO 639-2 language code format.
  @param  ControllerName        A pointer to the Unicode string to return.
                                This Unicode string is the name of the
                                controller specified by ControllerHandle and
                                ChildHandle in the language specified by
                                Language from the point of view of the driver
                                specified by This.

  @retval EFI_SUCCESS           The Unicode string for the user readable name in
                                the language specified by Language for the
                                driver specified by This was returned in
                                DriverName.
  @retval EFI_INVALID_PARAMETER ControllerHandle is not a valid EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER ChildHandle is not NULL and it is not a valid
                                EFI_HANDLE.
  @retval EFI_INVALID_PARAMETER Language is NULL.
  @retval EFI_INVALID_PARAMETER ControllerName is NULL.
  @retval EFI_UNSUPPORTED       The driver specified by This is not currently
                                managing the controller specified by
                                ControllerHandle and ChildHandle.
  @retval EFI_UNSUPPORTED       The driver specified by This does not support
                                the language specified by Language.

**/
EFI_STATUS
EFIAPI
UsbJoyStickComponentNameGetControllerName (
  IN  EFI_COMPONENT_NAME_PROTOCOL                     *This,
  IN  EFI_HANDLE                                      ControllerHandle,
  IN  EFI_HANDLE                                      ChildHandle        OPTIONAL,
  IN  CHAR8                                           *Language,
  OUT CHAR16                                          **ControllerName
  );



//
// Functions of Simple Text Input Protocol
//
/**
  Reset the input device and optionally run diagnostics

  There are 2 types of reset for USB keyboard.
  For non-exhaustive reset, only keyboard buffer is cleared.
  For exhaustive reset, in addition to clearance of keyboard buffer, the hardware status
  is also re-initialized.

  @param  This                 Protocol instance pointer.
  @param  ExtendedVerification Driver may perform diagnostics on reset.

  @retval EFI_SUCCESS          The device was reset.
  @retval EFI_DEVICE_ERROR     The device is not functioning properly and could not be reset.

**/
EFI_STATUS
EFIAPI
USBJoyStickReset (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  IN  BOOLEAN                          ExtendedVerification
  );

/**
  Reads the next keystroke from the input device.

  @param  This                 The EFI_SIMPLE_TEXT_INPUT_PROTOCOL instance.
  @param  Key                  A pointer to a buffer that is filled in with the keystroke
                               information for the key that was pressed.

  @retval EFI_SUCCESS          The keystroke information was returned.
  @retval EFI_NOT_READY        There was no keystroke data available.
  @retval EFI_DEVICE_ERROR     The keystroke information was not returned due to
                               hardware errors.

**/
EFI_STATUS
EFIAPI
USBJoyStickReadKeyStroke (
  IN  EFI_SIMPLE_TEXT_INPUT_PROTOCOL   *This,
  OUT EFI_INPUT_KEY                    *Key
  );

//
// Simple Text Input Ex protocol functions
//
/**
  Resets the input device hardware.

  The Reset() function resets the input device hardware. As part
  of initialization process, the firmware/device will make a quick
  but reasonable attempt to verify that the device is functioning.
  If the ExtendedVerification flag is TRUE the firmware may take
  an extended amount of time to verify the device is operating on
  reset. Otherwise the reset operation is to occur as quickly as
  possible. The hardware verification process is not defined by
  this specification and is left up to the platform firmware or
  driver to implement.

  @param This                 A pointer to the EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL instance.

  @param ExtendedVerification Indicates that the driver may perform a more exhaustive
                              verification operation of the device during reset.

  @retval EFI_SUCCESS         The device was reset.
  @retval EFI_DEVICE_ERROR    The device is not functioning correctly and could not be reset.

**/
EFI_STATUS
EFIAPI
USBJoyStickResetEx (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN BOOLEAN                            ExtendedVerification
  );

/**
  Reads the next keystroke from the input device.

  @param  This                   Protocol instance pointer.
  @param  KeyData                A pointer to a buffer that is filled in with the keystroke
                                 state data for the key that was pressed.

  @retval EFI_SUCCESS            The keystroke information was returned.
  @retval EFI_NOT_READY          There was no keystroke data available.
  @retval EFI_DEVICE_ERROR       The keystroke information was not returned due to
                                 hardware errors.
  @retval EFI_INVALID_PARAMETER  KeyData is NULL.

**/
EFI_STATUS
EFIAPI
USBJoyStickReadKeyStrokeEx (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL *This,
  OUT EFI_KEY_DATA                      *KeyData
  );

/**
  Set certain state for the input device.

  @param  This                    Protocol instance pointer.
  @param  KeyToggleState          A pointer to the EFI_KEY_TOGGLE_STATE to set the
                                  state for the input device.

  @retval EFI_SUCCESS             The device state was set appropriately.
  @retval EFI_DEVICE_ERROR        The device is not functioning correctly and could
                                  not have the setting adjusted.
  @retval EFI_UNSUPPORTED         The device does not support the ability to have its state set.
  @retval EFI_INVALID_PARAMETER   KeyToggleState is NULL.

**/
EFI_STATUS
EFIAPI
USBJoyStickSetState (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN EFI_KEY_TOGGLE_STATE               *KeyToggleState
  );

/**
  Register a notification function for a particular keystroke for the input device.

  @param  This                        Protocol instance pointer.
  @param  KeyData                     A pointer to a buffer that is filled in with
                                      the keystroke information for the key that was
                                      pressed. If KeyData.Key, KeyData.KeyState.KeyToggleState
                                      and KeyData.KeyState.KeyShiftState are 0, then any incomplete
                                      keystroke will trigger a notification of the KeyNotificationFunction.
  @param  KeyNotificationFunction     Points to the function to be called when the key
                                      sequence is typed specified by KeyData. This notification function
                                      should be called at <=TPL_CALLBACK.
  @param  NotifyHandle                Points to the unique handle assigned to the registered notification.

  @retval EFI_SUCCESS                 The notification function was registered successfully.
  @retval EFI_OUT_OF_RESOURCES        Unable to allocate resources for necessary data structures.
  @retval EFI_INVALID_PARAMETER       KeyData or NotifyHandle or KeyNotificationFunction is NULL.

**/
EFI_STATUS
EFIAPI
USBJoyStickRegisterKeyNotify (
  IN  EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN  EFI_KEY_DATA                       *KeyData,
  IN  EFI_KEY_NOTIFY_FUNCTION            KeyNotificationFunction,
  OUT VOID                               **NotifyHandle
  );

/**
  Remove a registered notification function from a particular keystroke.

  @param  This                      Protocol instance pointer.
  @param  NotificationHandle        The handle of the notification function being unregistered.

  @retval EFI_SUCCESS              The notification function was unregistered successfully.
  @retval EFI_INVALID_PARAMETER    The NotificationHandle is invalid
  @retval EFI_NOT_FOUND            Cannot find the matching entry in database.

**/
EFI_STATUS
EFIAPI
USBJoyStickUnregisterKeyNotify (
  IN EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL  *This,
  IN VOID                               *NotificationHandle
  );

/**
 * 
 * Initialize USB JoyStick device and all private structures.
 * @param UsbJoyStickDevice   The USB_JS_DEV instance.
 * 
 * @retval EFI_SUCCESS        Initialization is successful.
 * @retval EFI_DEVICE_ERROR   JoyStick initialization failed.
 * 
 * 
 **/
EFI_STATUS
InitUSBJoyStick (
  IN OUT USB_JS_DEV     *UsbJoyStickDevice
);



/**
 *
 * Uses USB I/O to check if the device is a USB JoyStick device.
 *
 * @para   UsbIo   Pointer to a USB I/O protocol instance.
 *
 *
 * @retval TRUE    Device is a USB JoyStick device.
 * @retval FALSE   Device is a not USB JoyStick device.
 */
BOOLEAN
IsUSBJoyStick(
  IN EFI_USB_IO_PROTOCOL           *UsbIo
  );

/**
  Handler function for USB JoyStick's asynchronous interrupt transfer.

  This function is the handler function for USB JoyStick's asynchronous interrupt transfer
  to manage the JoyStick. 

  @param  Data             A pointer to a buffer that is filled with key data which is
                           retrieved via asynchronous interrupt transfer.
  @param  DataLength       Indicates the size of the data buffer.
  @param  Context          Pointing to USB_JS_DEV instance.
  @param  Result           Indicates the result of the asynchronous interrupt transfer.

  @retval EFI_SUCCESS      Asynchronous interrupt transfer is handled successfully.
  @retval EFI_DEVICE_ERROR Hardware error occurs.

**/
EFI_STATUS
EFIAPI
JoyStickHandler (
  IN  VOID          *Data,
  IN  UINTN         DataLength,
  IN  VOID          *Context,
  IN  UINT32        Result
  );

#endif
