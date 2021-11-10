/** @file
 *
 *
 * 
 */


#include "JoyStick.h"

EFI_DRIVER_BINDING_PROTOCOL gUsbJoyStickDriverBinding = {
	USBJoyStickDriverBindingSupported,
	USBJoyStickDriverBindingStart,
	USBJoyStickDriverBindingStop,
	0xa,
	NULL,
	NULL
};


/**
  Entrypoint of USB Keyboard Driver.

  This function is the entrypoint of USB Keyboard Driver. It installs Driver Binding
  Protocols together with Component Name Protocols.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
USBJoyStickDriverBindingEntryPoint (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS              Status;

  Status = EfiLibInstallDriverBindingComponentName2 (
             ImageHandle,
             SystemTable,
             &gUsbJoyStickDriverBinding,
             ImageHandle,
             &gUsbJoyStickComponentName,
             &gUsbJoyStickComponentName2
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Check whether USB JoyStick driver supports this device.

  @param  This                   The USB JoyStick driver binding protocol.
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
  )
{
  EFI_STATUS          Status;
  EFI_USB_IO_PROTOCOL *UsbIo;

  //
  // Check if USB I/O Protocol is attached on the controller handle.
  //
  Status = gBS->OpenProtocol (
                  Controller,
                  &gEfiUsbIoProtocolGuid,
                  (VOID **) &UsbIo,
                  This->DriverBindingHandle,
                  Controller,
                  EFI_OPEN_PROTOCOL_BY_DRIVER
                  );
  if (EFI_ERROR (Status)) {
	  DEBUG((EFI_D_ERROR,"[JoyStick Driver] Not Supported \r\n")); 
    return Status;
  }

  //
  // Use the USB I/O Protocol interface to check whether Controller is
  // a JoyStick device that can be managed by this driver.
  //
  Status = EFI_SUCCESS;

  if (!IsUSBJoyStick (UsbIo)) {
	  DEBUG((EFI_D_ERROR,"[JoyStick Driver] Not Supported \r\n")); 
	  Status = EFI_UNSUPPORTED;
  }
  else{
          DEBUG((EFI_D_ERROR,"[JoyStick Driver] Supported \r\n"));
  }

  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
         );

  return Status;
 }

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
  )
{
      EFI_STATUS                    Status;
      EFI_USB_IO_PROTOCOL           *UsbIo;
      USB_JS_DEV                    *UsbJoyStickDevice;
      UINT8                         EndpointNumber;
      UINT8                         InEndpointAddr;
      UINT8                         InPollingInterval;
      UINT8                         InPacketSize;
      UINT8                         OutEndpointAddr;
      EFI_USB_ENDPOINT_DESCRIPTOR   EndpointDescriptor;
      BOOLEAN                       Found;
      EFI_TPL                       OldTpl;
      UINT32                        TransferStatus;
      
      OldTpl = gBS->RaiseTPL (TPL_CALLBACK);

      Status = gBS->OpenProtocol (
		      Controller,
		      &gEfiUsbIoProtocolGuid,
		      (VOID **)&UsbIo,
		      This->DriverBindingHandle,
		      Controller,
		      EFI_OPEN_PROTOCOL_BY_DRIVER
		      );
      if(EFI_ERROR (Status)){
              DEBUG((EFI_D_ERROR,"[JoyStick Driver] Protocol Guid ProtocolError \r\n"));
	      return Status;
      }

      UsbJoyStickDevice = AllocateZeroPool (sizeof (USB_JS_DEV));
      //ASSERT (UsbKeyboardDevice != NULL);
      
      Status = gBS->OpenProtocol(
		      Controller,
		      &gEfiDevicePathProtocolGuid,
		      (VOID **)&UsbJoyStickDevice->DevicePath,
		      This->DriverBindingHandle,
		      Controller,
		      EFI_OPEN_PROTOCOL_GET_PROTOCOL
		      );

      if (EFI_ERROR (Status)) {
              DEBUG((EFI_D_ERROR,"[JoyStick Driver]Device Path Protocol Error \r\n"));
	     return Status;
      }
      
      UsbJoyStickDevice->UsbIo = UsbIo;      

      UsbIo->UsbGetInterfaceDescriptor (
		      UsbIo,
		      &UsbJoyStickDevice->InterfaceDescriptor
		      );

      EndpointNumber = UsbJoyStickDevice->InterfaceDescriptor.NumEndpoints;
      DEBUG((EFI_D_ERROR,"Endpoint Number: %x\r\n",EndpointNumber));
      
      Found = FALSE;
      for(UINT8 i=0; i < EndpointNumber; i++)
      {
	      UsbIo->UsbGetEndpointDescriptor(
			      UsbIo,
			      i,
			      &EndpointDescriptor
			      );
	      if(((EndpointDescriptor.Attributes & (BIT0|BIT1))== USB_ENDPOINT_INTERRUPT) &&
		      ((EndpointDescriptor.EndpointAddress & USB_ENDPOINT_DIR_IN)==0)){
          Found = TRUE;
          DEBUG((EFI_D_ERROR,"Out Endpoint Address: %x\r\n",EndpointDescriptor.EndpointAddress));
		      CopyMem(&UsbJoyStickDevice->IntOutEndpointDescriptor,&EndpointDescriptor,sizeof(EndpointDescriptor));
	      }
	      if(((EndpointDescriptor.Attributes & (BIT0|BIT1))== USB_ENDPOINT_INTERRUPT) &&
		      ((EndpointDescriptor.EndpointAddress & USB_ENDPOINT_DIR_IN)!=0)){
          Found = TRUE;
          DEBUG((EFI_D_ERROR,"In Endpoint Address: %x\r\n",EndpointDescriptor.EndpointAddress));
		      CopyMem(&UsbJoyStickDevice->IntInEndpointDescriptor,&EndpointDescriptor,sizeof(EndpointDescriptor));
	      }
      }

      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
        EFI_PROGRESS_CODE,
        (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_DETECTED),
        UsbJoyStickDevice->DevicePath
      );
      
      if(Found == FALSE)
      {
        DEBUG((EFI_D_ERROR,"No Endpoint Found\r\n"));
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      UsbJoyStickDevice->Signature                         = USB_JS_DEV_SIGNATURE;
      UsbJoyStickDevice->SimpleInput.Reset                 = USBJoyStickReset;
      UsbJoyStickDevice->SimpleInput.ReadKeyStroke         = USBJoyStickReadKeyStroke;
      
      UsbJoyStickDevice->SimpleInputEx.Reset               = USBJoyStickResetEx;
      UsbJoyStickDevice->SimpleInputEx.ReadKeyStrokeEx     = USBJoyStickReadKeyStrokeEx;
      UsbJoyStickDevice->SimpleInputEx.SetState            = USBJoyStickSetState;
      UsbJoyStickDevice->SimpleInputEx.RegisterKeyNotify   = USBJoyStickRegisterKeyNotify;
      UsbJoyStickDevice->SimpleInputEx.UnregisterKeyNotify = USBJoyStickUnregisterKeyNotify;




      Status = gBS->InstallMultipleProtocolInterfaces (
                   &Controller,
                   &gEfiSimpleTextInProtocolGuid,
                   &UsbJoyStickDevice->SimpleInput,
                   &gEfiSimpleTextInputExProtocolGuid,
                   &UsbJoyStickDevice->SimpleInputEx,
                   NULL
      );
      if (EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"InstallMultipleProtocolInterface Failed!\r\n"));        
        Status = EFI_UNSUPPORTED;
        return Status;
      }
      UsbJoyStickDevice->ControllerHandle = Controller;
      
      Status = UsbJoyStickDevice->SimpleInputEx.Reset (
                   &UsbJoyStickDevice->SimpleInputEx,
                   TRUE
      );
      if(EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"SimpleInputEx Reset Failed\r\n"));
        gBS->UninstallMultipleProtocolInterfaces(
                   Controller,
                   &gEfiSimpleTextOutProtocolGuid,
                   &UsbJoyStickDevice->SimpleInput,
                   &gEfiSimpleTextOutProtocolGuid,
                   &UsbJoyStickDevice->SimpleInputEx,
                   NULL
        );
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      OutEndpointAddr   = UsbJoyStickDevice->IntOutEndpointDescriptor.EndpointAddress;
      DEBUG((EFI_D_ERROR,"Interrupt Out Endpoint Address: %x\r\n",OutEndpointAddr));

      InEndpointAddr    = UsbJoyStickDevice->IntInEndpointDescriptor.EndpointAddress;
      InPollingInterval = UsbJoyStickDevice->IntInEndpointDescriptor.Interval;
      InPacketSize      = (UINT8) (UsbJoyStickDevice->IntInEndpointDescriptor.MaxPacketSize);

      DEBUG((EFI_D_ERROR,"Interrupt In Endpoint Address: %x, Interval: %x, PacketSize: %x\r\n",InEndpointAddr,InPollingInterval,InPacketSize));
      
      UINT8 *IntOutData;
      UINTN IntOutSize = 64;
      IntOutData = AllocateZeroPool (64);
      IntOutData[0]    = 0x80;
      IntOutData[1]    = 0x02;  
      TransferStatus   = 0;

      Status = UsbIo->UsbSyncInterruptTransfer (
                   UsbIo,
                   OutEndpointAddr,
                   IntOutData,
                   &IntOutSize,
                   0,
                   &TransferStatus
      );
      if(EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"Send Interrupt Transfer failed\r\n"));
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      Status = UsbIo->UsbSyncInterruptTransfer (
                   UsbIo,
                   InEndpointAddr,
                   IntOutData,
                   &IntOutSize,
                   0,
                   &TransferStatus
      );
      if(EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"Receive Interrupt Transfer failed\r\n"));
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      IntOutData[0]    = 0x80;
      IntOutData[1]    = 0x04; 
      Status = UsbIo->UsbSyncInterruptTransfer (
                   UsbIo,
                   OutEndpointAddr,
                   IntOutData,
                   &IntOutSize,
                   0,
                   &TransferStatus
      );
      if(EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"Send Interrupt Transfer failed\r\n"));
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      Status = UsbIo->UsbSyncInterruptTransfer (
                   UsbIo,
                   InEndpointAddr,
                   IntOutData,
                   &IntOutSize,
                   0,
                   &TransferStatus
      );
      if(EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"Receive Interrupt Transfer failed\r\n"));
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      FreePool(IntOutData);
      
      Status = UsbIo->UsbAsyncInterruptTransfer (
                   UsbIo,
                   InEndpointAddr,
                   TRUE,
                   InPollingInterval,
                   InPacketSize,
                   JoyStickHandler,
                   UsbJoyStickDevice
      );

      if(EFI_ERROR(Status))
      {
        DEBUG((EFI_D_ERROR,"Submit Async Interrupt Transfer failed\r\n"));
        Status = EFI_UNSUPPORTED;
        return Status;
      }

      UsbJoyStickDevice->ControllerNameTable = NULL;
      AddUnicodeString2 (
        "eng",
        gUsbJoyStickComponentName2.SupportedLanguages,
        &UsbJoyStickDevice->ControllerNameTable,
        L"Nintendo Switch Pro Controller",
        TRUE
      );
      AddUnicodeString2(
        "en",
        gUsbJoyStickComponentName2.SupportedLanguages,
        &UsbJoyStickDevice->ControllerNameTable,
        L"Nintendo Switch Pro Controller",
        FALSE
      );
      gBS->RestoreTPL(OldTpl);

      return EFI_SUCCESS;
}

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
  )
{
  EFI_STATUS                      Status;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL  *SimpleInput;
  USB_JS_DEV                      *UsbJoyStickDevice;

  Status = gBS->OpenProtocol (
              Controller,
              &gEfiSimpleTextInProtocolGuid,
              (VOID **) &SimpleInput,
              This->DriverBindingHandle,
              Controller,
              EFI_OPEN_PROTOCOL_GET_PROTOCOL
  );
  if(EFI_ERROR  (Status))  {
       return EFI_UNSUPPORTED;
  }

  Status = gBS->OpenProtocol (
              Controller,
              &gEfiSimpleTextInputExProtocolGuid,
              NULL,
              This->DriverBindingHandle,
              Controller,
              EFI_OPEN_PROTOCOL_TEST_PROTOCOL
  );
  if(EFI_ERROR  (Status))  {
       return EFI_UNSUPPORTED;
  }

  UsbJoyStickDevice = USB_JS_DEV_FROM_THIS (SimpleInput);

  UsbJoyStickDevice->UsbIo->UsbAsyncInterruptTransfer (
                      UsbJoyStickDevice->UsbIo,
                      UsbJoyStickDevice->IntInEndpointDescriptor.EndpointAddress,
                      FALSE,
                      UsbJoyStickDevice->IntInEndpointDescriptor.Interval,
                      0,
                      NULL,
                      NULL
  );
  
  gBS->CloseProtocol (
         Controller,
         &gEfiUsbIoProtocolGuid,
         This->DriverBindingHandle,
         Controller
  );

  Status = gBS->UninstallMultipleProtocolInterfaces (
                Controller,
                &gEfiSimpleTextInProtocolGuid,
                &UsbJoyStickDevice->SimpleInput,
                &gEfiSimpleTextInputExProtocolGuid,
                &UsbJoyStickDevice->SimpleInputEx,
                NULL
  );
  if(UsbJoyStickDevice->ControllerNameTable !=NULL)
  {
    FreeUnicodeStringTable (UsbJoyStickDevice->ControllerNameTable);
  }
  FreePool (UsbJoyStickDevice);



  return Status;

}

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
  )
  {
    EFI_STATUS       Status;
    USB_JS_DEV       *UsbJoyStickDevice;

    UsbJoyStickDevice = USB_JS_DEV_FROM_THIS (This);
    REPORT_STATUS_CODE_WITH_DEVICE_PATH (
            EFI_ERROR_CODE | EFI_ERROR_MINOR,
            (EFI_PERIPHERAL_KEYBOARD | EFI_P_PC_RESET),
            UsbJoyStickDevice->DevicePath
    );

    if(!ExtendedVerification) {
          REPORT_STATUS_CODE_WITH_DEVICE_PATH (
            EFI_PROGRESS_CODE,
            (EFI_PERIPHERAL_KEYBOARD | EFI_P_KEYBOARD_PC_CLEAR_BUFFER),
            UsbJoyStickDevice->DevicePath
    );

        // Clear buffer

    return EFI_SUCCESS;
    }

    Status = InitUSBJoyStick(UsbJoyStickDevice);
    if(EFI_ERROR(Status)){
      return EFI_DEVICE_ERROR;
    }
    return EFI_SUCCESS;

  }

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
  )
  {
    return EFI_SUCCESS;

  }

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
  )
  {
    EFI_STATUS          Status;
    USB_JS_DEV          *UsbJoyStickDevice;

    UsbJoyStickDevice = TEXT_INPUT_EX_USB_JS_DEV_FROM_THIS (This);

    Status = UsbJoyStickDevice->SimpleInput.Reset (&UsbJoyStickDevice->SimpleInput,ExtendedVerification);
    if(EFI_ERROR(Status)){
      return EFI_DEVICE_ERROR;
    }

    return EFI_SUCCESS;
  }

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
  )
  {
    return EFI_SUCCESS;
  }

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
  )
  {
    return EFI_SUCCESS;
  }

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
  )
  {
    return EFI_SUCCESS;
  }

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
  )
  {
    return EFI_SUCCESS;
  }


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
)
{
  UINT16       ConfigValue;
  UINT8        Protocol;
  EFI_STATUS   Status;
  UINT32       TransferResult;

  REPORT_STATUS_CODE_WITH_DEVICE_PATH (
    EFI_PROGRESS_CODE,
    (EFI_PERIPHERAL_KEYBOARD|EFI_P_KEYBOARD_PC_SELF_TEST),
    UsbJoyStickDevice->DevicePath
  );
  DEBUG((EFI_D_INFO,"JoyStick Init"));

  Status = UsbGetConfiguration (
    UsbJoyStickDevice->UsbIo,
    &ConfigValue,
    &TransferResult
  );

  if(EFI_ERROR(Status)){
    DEBUG((EFI_D_INFO,"Get Configuration Failed"));
    return Status;
  }

  UsbGetProtocolRequest (
    UsbJoyStickDevice->UsbIo,
    UsbJoyStickDevice->InterfaceDescriptor.InterfaceNumber,
    &Protocol
  );

  if(Protocol!= 0)
  {
    UsbSetProtocolRequest (
      UsbJoyStickDevice->UsbIo,
      UsbJoyStickDevice->InterfaceDescriptor.InterfaceNumber,
      0
    );
  }

  ZeroMem (UsbJoyStickDevice->LastReport,sizeof(UINT8)*64);
  return EFI_SUCCESS;
}


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
  )
{
  EFI_STATUS                       Status;
  EFI_USB_DEVICE_DESCRIPTOR        DeviceDescriptor;

  //Get the device VID PID
  
  Status = UsbIo->UsbGetDeviceDescriptor(
		  UsbIo,
		  &DeviceDescriptor
		  );
  if(EFI_ERROR(Status)){
	  return FALSE;
  }
  
  DEBUG((EFI_D_INFO,"Vendor ID = 0x%04x \r\n", DeviceDescriptor.IdVendor));
  DEBUG((EFI_D_INFO,"Product ID = 0x%04x \r\n", DeviceDescriptor.IdProduct));
  
  if(DeviceDescriptor.IdVendor == NINTENDO_HID &&
     DeviceDescriptor.IdProduct == JOYSTICK_PID
    ){
	  return TRUE;
  }
  return FALSE;

}

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
  )
  {
    USB_JS_DEV            *UsbJoyStickDevice;
    EFI_USB_IO_PROTOCOL   *UsbIo;
    UINT32                UsbStatus;
    UINT8                 *CurrentReportData;
    UINT8                 *OldReportData;
    UINTN                 Index;

    UsbJoyStickDevice = (USB_JS_DEV *) Context;
    UsbIo             = UsbJoyStickDevice->UsbIo;
    
    if(Result != EFI_USB_NOERROR)
    {
      REPORT_STATUS_CODE_WITH_DEVICE_PATH (
            EFI_ERROR_CODE | EFI_ERROR_MINOR,
            (EFI_PERIPHERAL_KEYBOARD | EFI_P_EC_INPUT_ERROR),
            UsbJoyStickDevice->DevicePath
      );

      if(((Result & EFI_USB_ERR_STALL ) == EFI_USB_ERR_STALL)) {
        UsbClearEndpointHalt (
          UsbIo,
          UsbJoyStickDevice->IntInEndpointDescriptor.EndpointAddress,
          &UsbStatus
        );
      }

      UsbIo->UsbAsyncInterruptTransfer (
          UsbIo,
          UsbJoyStickDevice->IntInEndpointDescriptor.EndpointAddress,
          FALSE,
          0,
          0,
          NULL,
          NULL
      );
        return EFI_DEVICE_ERROR;    
    }

    CurrentReportData = (UINT8 *) Data;
    OldReportData     = UsbJoyStickDevice->LastReport;
    //Check for Button
    for (Index = 3; Index < 6; Index++)
    {
      if(OldReportData[Index]!=CurrentReportData[Index]){
          break;
      }
    }

    if (Index == 6)
    {
      return EFI_SUCCESS;
    }
    //DEBUG((EFI_D_INFO,"Button Data: 0x%02x\n\r", CurrentReportData[3]));

    
    if(((CurrentReportData[3] & 0x1)== 0x0 )&& ((BOOLEAN)(OldReportData[3] & (UINT8)0x1) == (UINT8)0x1))
    {
      Print(L"Y");
    }
    
    if(((CurrentReportData[3] & (UINT8)(0x1<<1)) == 0x0 )&& ((OldReportData[3] & (UINT8)(0x1<<1)) == (UINT8)(0x1<<1)))
    {
      Print(L"X");
    } 

    if(((CurrentReportData[3] & (UINT8)(0x1<<2)) == 0x0 )&& ((OldReportData[3] & (UINT8)(0x1<<2)) == (UINT8)(0x1<<2)))
    {
      Print(L"B");
    } 

    if(((CurrentReportData[3] & (UINT8)(0x1<<3)) == 0x0 )&& ((OldReportData[3] & (UINT8)(0x1<<3)) == (UINT8)(0x1<<3)))
    {
      Print(L"A");
    } 
    /*
    if(((CurrentReportData[5] & (UINT8)(0x1<<4)) == 0x0 )&& ((OldReportData[5] & (UINT8)(0x1<<4)) == (UINT8)(0x1<<4)))
    {
      Print(L"SR");
    }
    
    if(((CurrentReportData[5] & (UINT8)(0x1<<5)) == 0x0 )&& ((OldReportData[5] & (UINT8)(0x1<<5)) == (UINT8)(0x1<<5)))
    {
      Print(L"SL");
    } 
    */
//Update last report
    for (int i=0;i<64;i++)
    {
       UsbJoyStickDevice->LastReport[i]=CurrentReportData[i];
    }
    


    return EFI_SUCCESS;
  }
