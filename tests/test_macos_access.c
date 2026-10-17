/* Fake USB vtable only: no registry enumeration or connected-device access. */
#include "../src/macos.c"
#include <assert.h>

static unsigned opens,closes,releases,seizes,requests,cases;
static UInt16 vendor,product;
static IOReturn open_result,identity_result,request_result;
static IOReturn get_vendor(void *self,UInt16 *out){(void)self;*out=vendor;return identity_result;}
static IOReturn get_product(void *self,UInt16 *out){(void)self;*out=product;return kIOReturnSuccess;}
static IOReturn usb_open(void *self){(void)self;opens++;return open_result;}
static IOReturn usb_close(void *self){(void)self;closes++;return kIOReturnSuccess;}
static ULONG usb_release(void *self){(void)self;releases++;return 0;}
static IOReturn usb_seize(void *self){(void)self;seizes++;return kIOReturnError;}
static IOReturn usb_request(void *self,IOUSBDevRequestTO *req){
    (void)self;requests++;
    assert(req->bmRequestType==0xc0||req->bmRequestType==0x40);
    assert(req->noDataTimeout==3000&&req->completionTimeout==3000);
    req->wLenDone=req->wLength;
    return request_result;
}
static IOUSBDeviceInterface500 table={
    .Release=usb_release,.GetDeviceVendor=get_vendor,.GetDeviceProduct=get_product,
    .USBDeviceOpen=usb_open,.USBDeviceClose=usb_close,.USBDeviceOpenSeize=usb_seize,
    .DeviceRequestTO=usb_request
};
static IOUSBDeviceInterface500 *interface=&table;
static sp_macos setup(UInt16 pid,IOReturn result){
    cases++;opens=closes=releases=seizes=requests=0;
    vendor=0x2109;product=pid;open_result=result;
    identity_result=request_result=kIOReturnSuccess;
    return (sp_macos){.usb=&interface};
}
int main(void){
    char error[256];uint8_t data[32]={0};
    sp_macos m=setup(0x2822,kIOReturnExclusiveAccess);
    assert(!open_interface(0x2822,&m,error,sizeof error));
    assert(!m.opened&&m.shared_hub&&opens==1&&!seizes);
    assert(transfer(&m,0xc0,0xc4,0x0203,0x0010,data,32,3000)==32);
    assert(transfer(&m,0x40,0xf8,0xa2,0,NULL,0,3000)==0);
    assert(requests==2);
    /* Standard SET_CONFIGURATION and class/interface requests cannot leak
     * through the shared hub transport, including standard IN requests. */
    const uint8_t forbidden[]={0x00,0x80,0x20,0xa0,0x41,0xc1};
    for(size_t i=0;i<sizeof forbidden;i++)assert(transfer(&m,forbidden[i],9,1,0,NULL,0,3000)==-1);
    assert(requests==2);
    request_result=kIOReturnNotOpen;
    assert(transfer(&m,0xc0,0xc4,0x0203,0x0010,data,32,3000)==-1&&requests==3);
    sp_macos_close(&m);sp_macos_close(&m);
    assert(!closes&&releases==1&&!seizes&&!m.usb&&!m.shared_hub);

    m=setup(0x2822,kIOReturnSuccess);
    assert(!open_interface(0x2822,&m,error,sizeof error)&&m.opened&&!m.shared_hub);
    sp_macos_close(&m);sp_macos_close(&m);
    assert(closes==1&&releases==1&&!seizes);

    m=setup(0x8886,kIOReturnSuccess);
    assert(!open_interface(0x8886,&m,error,sizeof error)&&m.opened&&!m.shared_hub);
    sp_macos_close(&m);assert(closes==1&&releases==1);

    m=setup(0x8886,kIOReturnExclusiveAccess);
    assert(open_interface(0x8886,&m,error,sizeof error)==-1&&!m.usb);
    sp_macos_close(&m);assert(!closes&&releases==1&&!requests&&!seizes);

    const IOReturn fatal[]={kIOReturnNotPermitted,kIOReturnNoDevice,kIOReturnError};
    for(size_t i=0;i<sizeof fatal/sizeof fatal[0];i++){
        m=setup(0x2822,fatal[i]);
        assert(open_interface(0x2822,&m,error,sizeof error)==-1&&!m.usb);
        sp_macos_close(&m);assert(!closes&&releases==1&&!requests&&!seizes);
    }
    m=setup(0x2822,kIOReturnExclusiveAccess);identity_result=kIOReturnNoDevice;
    assert(open_interface(0x2822,&m,error,sizeof error)==-1&&!opens&&releases==1);
    m=setup(0x2822,kIOReturnExclusiveAccess);vendor=0x1234;
    assert(open_interface(0x2822,&m,error,sizeof error)==-1&&!opens&&releases==1);
    m=setup(0x2822,kIOReturnExclusiveAccess);
    assert(open_interface(0x8886,&m,error,sizeof error)==-1&&!opens&&releases==1);
    printf("Native USB access policy: %u scenarios passed; fake USB only, no hardware access.\n",cases);
    return 0;
}
