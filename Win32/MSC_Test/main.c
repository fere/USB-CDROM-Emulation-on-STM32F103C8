#include <windows.h>
#include <setupapi.h>
#include <stddef.h>
#include <stdio.h>
#include <tchar.h>
#include <initguid.h>
#include <ntddscsi.h>
#include <cfgmgr32.h>
#include <winioctl.h>


#define SPT_SENSE_LENGTH                    32
#define SPTWB_DATA_LENGTH                   2048

#define CDB6GENERIC_LENGTH                  6
#define CDB10GENERIC_LENGTH                 10

#define SCSIOP_INQUIRY                      0x12
#define SCSIOP_READ_CAPACITY                0x25

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
  SCSI_PASS_THROUGH Spt;
  ULONG             Filler; /* realign buffers to double word boundary */
  UCHAR             Sense[SPT_SENSE_LENGTH];
  UCHAR             Data[SPTWB_DATA_LENGTH];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

static int DoScsiInquiry(HANDLE hDevice,TCHAR *vdrName,TCHAR *devName)
{
  SCSI_PASS_THROUGH_WITH_BUFFERS SptWB;
  DWORD returned;
  BOOL Ret;
  int RetCode=0, length;

  ZeroMemory(&SptWB,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 0;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = CDB6GENERIC_LENGTH;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  SptWB.Spt.DataTransferLength = 192;
  SptWB.Spt.TimeOutValue = 2;
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);

  SptWB.Spt.Cdb[0] = SCSIOP_INQUIRY;
  SptWB.Spt.Cdb[4] = 192;

  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) + SptWB.Spt.DataTransferLength;
  
  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        sizeof(SCSI_PASS_THROUGH),
                        &SptWB,
                        length,
                        &returned,
                        NULL);
  if (!Ret)
  {
    RetCode=GetLastError();
    return RetCode;
  }

  /* Don't compile this code with UNICODE support. */
  if (vdrName)
  {
    memcpy(vdrName, (char *)SptWB.Data+8, 8);
    vdrName[8] = 0;
  }
  if (devName)
  {
    memcpy(devName, (char *)SptWB.Data+16, 20);
    devName[20] = 0;
  }

  /* Try some SCSI commands. */
  /* First: READ_CCAPACITY */
  ZeroMemory(&SptWB,offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 0;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = CDB10GENERIC_LENGTH;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  SptWB.Spt.DataTransferLength = 8;
  SptWB.Spt.TimeOutValue = 2;
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);

  SptWB.Spt.Cdb[0] = SCSIOP_READ_CAPACITY;

  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) + SptWB.Spt.DataTransferLength;

  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        sizeof(SCSI_PASS_THROUGH),
                        &SptWB,
                        length,
                        &returned,
                        NULL);
  if (!Ret)
  {
    RetCode=GetLastError();
    return RetCode;
  }

  /* Second: READ(10) */
  ZeroMemory(&SptWB,offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 0;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = CDB10GENERIC_LENGTH;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  SptWB.Spt.DataTransferLength = 2048*0x01;
  SptWB.Spt.TimeOutValue = 5;
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);

  SptWB.Spt.Cdb[0] = 0x28;
  SptWB.Spt.Cdb[1] = 0x00;
  SptWB.Spt.Cdb[2] = 0x00;
  SptWB.Spt.Cdb[3] = 0x00;
  SptWB.Spt.Cdb[4] = 0x00;
  SptWB.Spt.Cdb[5] = 0x10;
  SptWB.Spt.Cdb[6] = 0x00;
  SptWB.Spt.Cdb[7] = 0x00;
  SptWB.Spt.Cdb[8] = 0x01;
  SptWB.Spt.Cdb[9] = 0x00;

  memset(SptWB.Data, 0x4D, SPTWB_DATA_LENGTH);
  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) + SptWB.Spt.DataTransferLength;

  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        length,
                        &SptWB,
                        length,
                        &returned,
                        FALSE);
  if (!Ret)
  {
    RetCode=GetLastError();
    return RetCode;
  }

  /* Third: GET_CHIP_ID (Custom Command) */
  ZeroMemory(&SptWB,offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 0;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = CDB10GENERIC_LENGTH;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  SptWB.Spt.DataTransferLength = 32;
  SptWB.Spt.TimeOutValue = 5;
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);

  SptWB.Spt.Cdb[0] = 0xFF;
  SptWB.Spt.Cdb[1] = 0x00;
  SptWB.Spt.Cdb[2] = 0x00;
  SptWB.Spt.Cdb[3] = 0x00;
  SptWB.Spt.Cdb[4] = 0x00;
  SptWB.Spt.Cdb[5] = 0x00;
  SptWB.Spt.Cdb[6] = 0x00;
  SptWB.Spt.Cdb[7] = 0x00;
  SptWB.Spt.Cdb[8] = 0x00;
  SptWB.Spt.Cdb[9] = 0x00;

  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) + SptWB.Spt.DataTransferLength;

  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        length,
                        &SptWB,
                        length,
                        &returned,
                        FALSE);
  if (!Ret)
  {
    RetCode=GetLastError();
    return RetCode;
  }

  return RetCode;
}

#define USB_NO_DEVICE                 0x00000000
#define USB_SPECIFY_DEVICE_NOT_FOUND  0x00000001
#define USB_SPECIFY_DEVICE_EXIST      0x00000002
#define USB_UNKNOWN_ERROR             0x000000FF

static BOOL GetDriveProperty(HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR pDevDesc)
{
  STORAGE_PROPERTY_QUERY Query;
  DWORD dwOutBytes;
  BOOL bResult;

  Query.PropertyId = StorageDeviceProperty;
  Query.QueryType = PropertyStandardQuery;

  bResult = DeviceIoControl(hDevice,
                            IOCTL_STORAGE_QUERY_PROPERTY,
                            &Query, sizeof(STORAGE_PROPERTY_QUERY),
                            pDevDesc, pDevDesc->Size,
                            &dwOutBytes,
                           (LPOVERLAPPED)NULL);

  return bResult;
}

int GetMscDeviceContext(int index,TCHAR *vdrName,TCHAR *devName,TCHAR *devPath)
{
  //GUID Guid;
  HANDLE hDevInfo,hDevice;
  SP_DEVINFO_DATA DevInfoData;
  SP_DEVICE_INTERFACE_DATA devInfoData;
  PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = NULL;
  STORAGE_DEVICE_DESCRIPTOR DevDesc;
  int retcode=0;
  DWORD length = 0, required;
  char buffer[256];

  vdrName[0] = devName[0] = devPath[0] = 0;

  //memcpy(&Guid,&GUID_DEVINTERFACE_CDROM,sizeof(GUID_CLASS_SCSI_USBCDROM));

  hDevInfo = SetupDiGetClassDevs(&GUID_DEVINTERFACE_CDROM,NULL,NULL,DIGCF_PRESENT|DIGCF_INTERFACEDEVICE);
  if (hDevInfo == INVALID_HANDLE_VALUE)
  {
    return USB_NO_DEVICE;
  }

  DevInfoData.cbSize = sizeof(DevInfoData);
  if (!SetupDiEnumDeviceInfo(hDevInfo, index, &DevInfoData))
  {
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_SPECIFY_DEVICE_NOT_FOUND;
  }

  devInfoData.cbSize = sizeof(devInfoData);
  if (!SetupDiEnumDeviceInterfaces(hDevInfo,0,&GUID_DEVINTERFACE_CDROM,index,&devInfoData))
  {
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_SPECIFY_DEVICE_NOT_FOUND;
  }

  SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInfoData, NULL, 0, &length, NULL);
  if (length > sizeof(buffer))
  {
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_UNKNOWN_ERROR;
  }

  memset(buffer, 0, sizeof(buffer));
  detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buffer;
  detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

  if (SetupDiGetDeviceInterfaceDetail(hDevInfo, &devInfoData, detailData, length, &required, NULL) == 0)
  {
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_UNKNOWN_ERROR;
  }

  hDevice = CreateFile(detailData->DevicePath,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
  if (hDevice == INVALID_HANDLE_VALUE )
  {
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_UNKNOWN_ERROR;
  }

  DevDesc.Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
  if (!GetDriveProperty(hDevice, &DevDesc))
  {
    CloseHandle(hDevice);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_UNKNOWN_ERROR;
  }
  if (DevDesc.BusType!=BusTypeUsb)
  {
    CloseHandle(hDevice);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_NO_DEVICE;
  }

  if (DoScsiInquiry(hDevice, vdrName, devName)==0)
  {
    _tcscpy(devPath,detailData->DevicePath);

    CloseHandle(hDevice);
    SetupDiDestroyDeviceInfoList(hDevInfo);
    return USB_SPECIFY_DEVICE_EXIST;
  }

  CloseHandle(hDevice);
  SetupDiDestroyDeviceInfoList(hDevInfo);
  return USB_NO_DEVICE;
}

// Finds the device interface for the CDROM drive with the given interface number.
DEVINST GetDrivesDevInstByDeviceNumber(DEVINST device, long DeviceNumber)
{
  const GUID *guid = &GUID_DEVINTERFACE_CDROM;

  // Retrieve a context structure for a device interface of a device information set.
  BYTE                             buf[1024];
  PSP_DEVICE_INTERFACE_DETAIL_DATA pspdidd = (PSP_DEVICE_INTERFACE_DETAIL_DATA)buf;
  SP_DEVICE_INTERFACE_DATA         spdid;
  SP_DEVINFO_DATA                  spdd;
  DWORD                            dwSize,i;

  // Get device interface info set handle
  // for all devices attached to system
  HDEVINFO hDevInfo = SetupDiGetClassDevs(guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
  if(hDevInfo == INVALID_HANDLE_VALUE)
    return 0;

  spdid.cbSize = sizeof(spdid);

  // Iterate through all the interfaces and try to match one based on
  // the device number.
  for(i = 0; SetupDiEnumDeviceInterfaces(hDevInfo,NULL,guid,i,&spdid); i++)
  {
    STORAGE_DEVICE_DESCRIPTOR DevDesc;
    STORAGE_DEVICE_NUMBER sdn;
    HANDLE hDrive;

    // Get the device path.
    dwSize = 0;
    SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, NULL, 0, &dwSize, NULL);
    if(dwSize == 0 || dwSize > sizeof(buf))
      continue;

    pspdidd->cbSize = sizeof(*pspdidd);
    ZeroMemory((PVOID)&spdd, sizeof(spdd));
    spdd.cbSize = sizeof(spdd);
    if(!SetupDiGetDeviceInterfaceDetail(hDevInfo, &spdid, pspdidd, dwSize, &dwSize, &spdd))
      continue;

    // Open the device.
    hDrive = CreateFile(pspdidd->DevicePath,GENERIC_READ|GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL, OPEN_EXISTING, 0, NULL);
    if(hDrive == INVALID_HANDLE_VALUE)
      continue;

    DevDesc.Size = sizeof(STORAGE_DEVICE_DESCRIPTOR);
    if (!GetDriveProperty(hDrive, &DevDesc))
    {
      CloseHandle(hDrive);
      continue;
    }
    if(DevDesc.BusType!=BusTypeUsb)
    {
      CloseHandle(hDrive);
      continue;
    }

    if (device != 0 && device == spdd.DevInst)
    {
      if (DoScsiInquiry(hDrive, NULL, NULL)==0)
      {
        CloseHandle(hDrive);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return spdd.DevInst;
      }
      continue;
    }

    /* Get the device number. */
    dwSize = 0;
    if( DeviceIoControl(hDrive,
                        IOCTL_STORAGE_GET_DEVICE_NUMBER,
                        NULL, 0, &sdn, sizeof(sdn),
                        &dwSize, NULL))
    {
      // Does it match?
      if(device == 0 && DeviceNumber == (long)sdn.DeviceNumber)
      {
        CloseHandle(hDrive);
        SetupDiDestroyDeviceInfoList(hDevInfo);
        return spdd.DevInst;
      }
    }

    CloseHandle(hDrive);
  }

  SetupDiDestroyDeviceInfoList(hDevInfo);
  return 0;
}

// Returns true if the given device instance belongs to the USB device with the given VID and PID.
DEVINST matchDevInstToUsbDevice(DEVINST device, DWORD vid, DWORD pid)
{
  // This is the string we will be searching for in the device harware IDs.
  TCHAR hwid[64];
  TCHAR *pszBuffer;
  LPTSTR pszDeviceID;
  ULONG ulLen,i;

  _stprintf(hwid, _T("VID_%04X&PID_%04X"), vid, pid);

  // Get a list of hardware IDs for all USB devices.
  CM_Get_Device_ID_List_Size(&ulLen, _T("USB"), CM_GETIDLIST_FILTER_ENUMERATOR);
  pszBuffer = (TCHAR *)GlobalAlloc(GMEM_ZEROINIT,ulLen*sizeof(TCHAR));
  CM_Get_Device_ID_List(_T("USB"), pszBuffer, ulLen, CM_GETIDLIST_FILTER_ENUMERATOR);

  // Iterate through the list looking for our ID.
  for(pszDeviceID = pszBuffer; *pszDeviceID; pszDeviceID += _tcslen(pszDeviceID) + 1)
  {
    // Some versions of Windows have the string in upper case and other versions have it
    // in lower case so just make it all upper.
    for(i = 0; pszDeviceID[i]; i++)
      pszDeviceID[i] = toupper(pszDeviceID[i]);

    if(_tcsstr(pszDeviceID, hwid))
    {
      // Found the device, now we want the grandchild device, which is the "generic volume"
      DEVINST MSDInst = 0;
      if(CR_SUCCESS == CM_Locate_DevNode(&MSDInst, pszDeviceID, CM_LOCATE_DEVNODE_NORMAL))
      {
        DEVINST DiskDriveInst = 0;
        if(CR_SUCCESS == CM_Get_Child(&DiskDriveInst, MSDInst, 0))
        {
          // Now compare the grandchild node against the given device instance.
          if(device != 0)
          {
            if(device == DiskDriveInst)
            {
              GlobalFree(pszBuffer);
              return DiskDriveInst;
            }
          }
          else
          {
            GlobalFree(pszBuffer);
            return DiskDriveInst;
          }
        }
      }
    }
  }

  GlobalFree(pszBuffer);
  return (DEVINST)0;
}

ULONG doScsiReadCapacity(HANDLE hDevice, long * bytesPerSector)
{
  SCSI_PASS_THROUGH_WITH_BUFFERS SptWB;
  ULONG returned = 0,length;
  BOOL Ret;

  ZeroMemory(&SptWB,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 0;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = CDB10GENERIC_LENGTH;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  SptWB.Spt.DataTransferLength = 8;
  SptWB.Spt.TimeOutValue = 2;
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);

  SptWB.Spt.Cdb[0] = SCSIOP_READ_CAPACITY;

  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) +  SptWB.Spt.DataTransferLength;

  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        sizeof(SCSI_PASS_THROUGH),
                        &SptWB,
                        length,
                        &returned,
                        NULL);
  if(!Ret)
  {
    return -1;
  }

  /* The LBA of last sector. */
  returned =  SptWB.Data[0]<<24;
  returned |= SptWB.Data[1]<<16;
  returned |= SptWB.Data[2]<<8;
  returned |= SptWB.Data[3]<<0;

  /* Bytes per sector. */
  if (bytesPerSector)
  {
    *bytesPerSector =  SptWB.Data[4]<<24;
    *bytesPerSector |= SptWB.Data[5]<<16;
    *bytesPerSector |= SptWB.Data[6]<<8;
    *bytesPerSector |= SptWB.Data[7]<<0;
  }

  return returned;
}

ULONG doScsiBurnCommand(HANDLE hDevice, void * pBlock, long lba, long * trueLength)
{
  SCSI_PASS_THROUGH_WITH_BUFFERS SptWB;
  ULONG returned=0,length=0;
  BOOL Ret;

  ZeroMemory(&SptWB,offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 1;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = 0x0A;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_OUT;
  SptWB.Spt.DataTransferLength = 2048;
  SptWB.Spt.TimeOutValue = 20;
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);
  SptWB.Spt.Cdb[0] = 0x2A;
  SptWB.Spt.Cdb[1] = 0;
  SptWB.Spt.Cdb[2] = (lba>>24)&0xFF;
  SptWB.Spt.Cdb[3] = (lba>>16)&0xFF;
  SptWB.Spt.Cdb[4] = (lba>>8 )&0xFF;
  SptWB.Spt.Cdb[5] = (lba>>0 )&0xFF;
  SptWB.Spt.Cdb[6] = 0;
  SptWB.Spt.Cdb[7] = 0;
  SptWB.Spt.Cdb[8] = 1;
  SptWB.Spt.Cdb[9] = 0;

  if (pBlock != NULL)
  {
    memcpy(&SptWB.Data, pBlock, SPTWB_DATA_LENGTH);
  }
  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) + SptWB.Spt.DataTransferLength;

  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        length,
                        &SptWB,
                        length,
                        &returned,
                        FALSE);
  if (!Ret)
  {
    _tprintf(_T("Write sector %x failure!\n"),lba);
    return -1;
  }

  ZeroMemory(&SptWB,offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data));
  SptWB.Spt.Length = sizeof(SCSI_PASS_THROUGH);
  SptWB.Spt.PathId = 0;
  SptWB.Spt.TargetId = 1;
  SptWB.Spt.Lun = 0;
  SptWB.Spt.CdbLength = 0x0A;
  SptWB.Spt.SenseInfoLength = 24;
  SptWB.Spt.DataIn = SCSI_IOCTL_DATA_IN;
  SptWB.Spt.DataTransferLength = 2048;
  SptWB.Spt.TimeOutValue = 5;
  SptWB.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data);
  SptWB.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Sense);
  SptWB.Spt.Cdb[0] = 0xFE;
  SptWB.Spt.Cdb[1] = 0;
  SptWB.Spt.Cdb[2] = (lba>>24)&0xFF;
  SptWB.Spt.Cdb[3] = (lba>>16)&0xFF;
  SptWB.Spt.Cdb[4] = (lba>>8 )&0xFF;
  SptWB.Spt.Cdb[5] = (lba>>0 )&0xFF;
  SptWB.Spt.Cdb[6] = 0;
  SptWB.Spt.Cdb[7] = 0;
  SptWB.Spt.Cdb[8] = 1;
  SptWB.Spt.Cdb[9] = 0;

  memset(SptWB.Data, 0x4D, SPTWB_DATA_LENGTH);
  length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,Data) + SptWB.Spt.DataTransferLength;

  Ret = DeviceIoControl(hDevice,
                        IOCTL_SCSI_PASS_THROUGH,
                        &SptWB,
                        length,
                        &SptWB,
                        length,
                        &returned,
                        FALSE);
  if (!Ret)
  {
    _tprintf(_T("Read sector %x failure!\n"),lba);
    return -1;
  }

  if (pBlock)
  {
    if (memcmp(pBlock, SptWB.Data, SPTWB_DATA_LENGTH)!=0)
    {
      _tprintf(_T("Write & Verify sector %x error!\n"),lba);
      return -1;
    }
  }

  return 1;
}

typedef struct _msc_context {
  TCHAR     vendor_name[64];
  TCHAR     device_name[64];
  TCHAR     device_path[128];
} msc_context;

static void writeISOFile(msc_context *hctx, TCHAR *fname)
{
  FILE *hnd = NULL;
  char buffer[2048*5];
  int fsize,ret,blk,maxlba;
  long bytesPerSector;

  HANDLE hDevice = CreateFile(hctx->device_path,GENERIC_READ | GENERIC_WRITE,FILE_SHARE_WRITE|FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
  if (hDevice == INVALID_HANDLE_VALUE )
  {
    DWORD dwRet = GetLastError();
    return;
  }

  if (_tcscmp(fname,_T("zero")) == 0)
  {
    _tprintf(_T("Write All Zero to the disc!\n"));
    hnd = NULL;
  }
  else
  if (_tcscmp(fname,_T("none")) == 0)
  {
    CloseHandle(hDevice);
    return;
  }
  else
  {
    _tprintf(_T("Write %s to the disc!\n"),fname);

    hnd = _tfopen(fname,_T("rb"));
    if (hnd == NULL)
    {
      _tprintf(_T("%s is not found!\n"),fname);
      CloseHandle(hDevice);
      return;
    }

    fseek(hnd,0,SEEK_END);
    fsize = ftell(hnd);
    _tprintf(_T("file length = %d - 0x%x bytes\n"),fsize,fsize);
    fseek(hnd,0,SEEK_SET);
  }

  maxlba = doScsiReadCapacity(hDevice, &bytesPerSector);
  _tprintf(_T("maxlba = %x, maxCapacity = %d bytes\n"),maxlba,(maxlba+1)*bytesPerSector);
  if (maxlba == 0)
  {
    _tprintf(_T("Error!!! The flash chip wasn't detected!\n"));
    if (hnd != NULL)
    {
      fclose(hnd);
    }
    CloseHandle(hDevice);
    return;
  }
  else
  if (hnd == NULL)
  {
    fsize = (maxlba+1)*bytesPerSector;
  }

  if (maxlba != -1)
  {
    blk = 0;
    while(fsize > 0)
    {
      if (blk == 0x10)
      {
        _tprintf(_T("\nWrite external flash!\n"));
      }
      if (hnd != NULL)
      {
        fread(buffer,2048,1,hnd);
      }
      else
      {
        memset(buffer,0,2048);
      }
      ret = doScsiBurnCommand(hDevice,buffer,blk,NULL);
      if (ret != 1)
      {
        _tprintf(_T("write block %d - 0x%x error!\n"),blk,blk);
        if (hnd != NULL)
        {
          fclose(hnd);
        }
        CloseHandle(hDevice);
        return;
      }
      _tprintf(_T("."));
      fsize -= 2048; blk++;
    }
  }
  else
  {
    _tprintf(_T("Can't exec SCSI_READ_CAPACITY command!\n"));
    _tprintf(_T("Device is error!\n"));
  }
  _tprintf(_T("\n"));

  if (hnd)
  {
    fclose(hnd);
  }
  CloseHandle(hDevice);
}

int main(int argc, char *argv[])
{
  int idx,rv;
  msc_context hctx;

  for(idx=0;idx<128;idx++)
  {
    if (!GetDrivesDevInstByDeviceNumber(matchDevInstToUsbDevice(0,1155,22314),idx))
    {
      continue;
    }

    rv = GetMscDeviceContext(idx,hctx.vendor_name,hctx.device_name,hctx.device_path);
    if (USB_SPECIFY_DEVICE_EXIST == rv)
    {
      _tprintf(_T("%s\n"),hctx.device_path);
      writeISOFile(&hctx, _T("medium.iso"));
      break;
    }
    else
    if (USB_SPECIFY_DEVICE_NOT_FOUND == rv)
    {
      _tprintf(_T("Device not found!\n"));
      return -1;
    }
  }

  return 0;
}
