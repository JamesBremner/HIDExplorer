#include "cHID.h"

#include <iostream>
#include <sstream>

static const std::wstring& myToWString( int i,
                                        bool hex = false )
{
    std::wstringstream ss;
    if( !hex )
        ss << i;
    else
        ss << std::hex << i;

    static std::wstring ws;
    ws = ss.str();
    return ws;
}
static const std::wstring& myStoWs( const std::string& s )
{
    static std::wstring ws( s.begin(), s.end() );
    return ws;
}

namespace raven
{
namespace hid
{

void cVectorHID::Populate()
{
    myHID.clear();

    GUID HID_GUID;
    HidD_GetHidGuid( & HID_GUID );
    HDEVINFO DeviceInfoSet = SetupDiGetClassDevs(
                                 &HID_GUID,
                                 NULL,NULL,
                                 DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    SP_DEVICE_INTERFACE_DATA DeviceInfoData;
    DeviceInfoData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
    int DeviceIndex = 0;
    while ( SetupDiEnumDeviceInterfaces(
                DeviceInfoSet,
                NULL,
                &HID_GUID,
                DeviceIndex,
                &DeviceInfoData))
    {
        DeviceIndex++;

        //Get the details with null values to get the required size of the buffer
        PSP_INTERFACE_DEVICE_DETAIL_DATA deviceDetail;
        ULONG requiredSize = 0;
        SetupDiGetDeviceInterfaceDetail (DeviceInfoSet,
                                         &DeviceInfoData,
                                         NULL, //interfaceDetail,
                                         0, //interfaceDetailSize,
                                         &requiredSize,
                                         0); //infoData))

        //Allocate the buffer
        deviceDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)malloc(requiredSize);
        deviceDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);

        //Fill the buffer with the device details
        if ( SetupDiGetDeviceInterfaceDetail (DeviceInfoSet,
                                              &DeviceInfoData,
                                              deviceDetail,
                                              requiredSize,
                                              &requiredSize,
                                              NULL))
        {
            myHID.push_back( std::shared_ptr< cHID> (
                                 new cHID( deviceDetail->DevicePath )));
        }

        free (deviceDetail);

    }
    if (DeviceInfoSet)
    {
        SetupDiDestroyDeviceInfoList(DeviceInfoSet);
    }

}

void cVectorHID::FilterOutNonCompliant()
{
    std::vector< std::shared_ptr< cHID > > temp;
    for( auto h : myHID )
    {
        if( h->getStatus()  && h->getManufacturer() != L"na" )
            temp.push_back( h );
    }
    myHID = temp;
}


cDumpHID::cDumpHID( cVectorHID & vHID )
{

    // column titles
    std::vector< std::wstring  > titles;
    titles.push_back( L"Manufacturer");
    titles.push_back( L"Product");
    titles.push_back( L"VendorID");
    titles.push_back( L"ProductID");
    titles.push_back( L"Usage Page");
    titles.push_back( L"Usage");
    titles.push_back( L"Nodes");
    titles.push_back( L"Buttons");
    titles.push_back( L"Values");

    myDump.push_back( titles );


    for( auto h : vHID )
    {
        std::vector< std::wstring  > line;

        line.push_back( std::wstring ( h->getManufacturer() ));
        line.push_back( std::wstring ( h->getProduct() ) );
        line.push_back( myToWString( h->getVendorID(), true ));
        line.push_back( myToWString( h->getProductID(), true ) );
        line.push_back( myToWString( h->getUsagePage() ) );
        line.push_back( myToWString( h->getUsage() ));
        line.push_back( myToWString( h->getNodes() ));
        line.push_back( myToWString( h->getButtonCount() ));
        line.push_back( myToWString( h->getValueCount() ));

        //line.push_back( myStoWs( h->getPath() ) )                            ;

        myDump.push_back( line );
    }

}



void cDumpHID::Text( std::wstring& text )
{
    std::wstringstream ss;
    int counter = 0;
    for( std::vector< std::wstring >& l : myDump )
    {

        if( ! counter )
            ss << "Index, ";
        else
            ss << counter << L": ";
        counter++;

        // loop over HID details
        for( std::wstring& s : l )
        {
            // display HID detail
            ss << s << L", ";
        }
        ss << L"\n";
    }
    text = ss.str();
}


cHID::cHID( const char * path )
    : myStatus( false )
    , myVendorID( -1 )
    , myProductID( -1 )
    , myManufacturer( L"na" )
    , myProduct( L"na" )

{
    myPath = path;

    HANDLE myHandle;

    try
    {
        myHandle = CreateFile(
                       myPath.c_str(),
                       GENERIC_READ | GENERIC_WRITE,
                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL,
                       OPEN_EXISTING,
                       FILE_ATTRIBUTE_NORMAL,
                       NULL);
        if( myHandle == INVALID_HANDLE_VALUE )
            return;

        wchar_t s[ 2560 ];

        if( HidD_GetManufacturerString(
                    myHandle,
                    s, 2500 ) )
            myManufacturer = s;

        if( HidD_GetProductString(
                    myHandle,
                    s, 2500 ) )
            myProduct = s;

        if(  HidD_GetPhysicalDescriptor(
                    myHandle,
                    s,2500) )
            myPhysical = s;
        else
            myPhysical = L"na";

        _HIDD_ATTRIBUTES attributes;
        attributes.Size = sizeof( _HIDD_ATTRIBUTES );
        if( ! HidD_GetAttributes(
                    myHandle,
                    &attributes ) )
        {
            throw std::runtime_error("HidD_GetAttributes failed");
        }
        myVendorID = attributes.VendorID;
        myProductID = attributes.ProductID;


        readCAPS( myHandle );

        myStatus = true;

    }

    catch( std::runtime_error& e )
    {
        std::cout << e.what() << std::endl;

    }

    CloseHandle( myHandle );

    SanityTest();
}

void cHID::SanityTest()
{
    if( ! myStatus )
        return;
    if( myManufacturer == L"na" )
        myStatus = false;

    int i = myPath.find("vid_") + 4;
    int j = myPath.find("&",i);
    if( i == -1 || j == -1 )
    {
        myStatus = false;
        return;
    }
    //std::cout << myPath << std::endl;
    //std::cout << myPath.substr(i,j-i) << std::endl;


    int x;
    std::stringstream ss;
    ss << std::hex << myPath.substr(i,j-i);
    ss >> x;
    //std::cout << x << std::endl;

    if( x != myVendorID )
        myStatus = false;
}

bool cHID::readCAPS( HANDLE myHandle )
{
    bool bRet = false;
    PHIDP_PREPARSED_DATA preparsed_data = 0;
    try
    {
        if( ! HidD_GetPreparsedData(
                    myHandle,
                    &preparsed_data
                ) )
            throw std::runtime_error("HidD_GetPreparsedData failed");
        if( HIDP_STATUS_SUCCESS != HidP_GetCaps(
                    preparsed_data,
                    &myCaps ) )
            throw std::runtime_error("HidP_GetCaps failed ");

        if( myCaps.NumberLinkCollectionNodes > 0  )
        {
            unsigned long lcount = 100;
            HIDP_LINK_COLLECTION_NODE LinkCollectionNodes[ lcount ];
            if( HIDP_STATUS_SUCCESS != HidP_GetLinkCollectionNodes(
                        LinkCollectionNodes,
                        &lcount,
                        preparsed_data
                    ) )
                throw std::runtime_error("HidP_GetLinkCollectionNodes failed");
            //HIDP_LINK_COLLECTION_NODE node = LinkCollectionNodes[0];
            // dbg = 0;
        }

        if( myCaps.NumberInputButtonCaps > 0 )
        {
            unsigned long bcount = myCaps.NumberInputButtonCaps;
            HIDP_BUTTON_CAPS buttons[ bcount ];
            if( HIDP_STATUS_SUCCESS != HidP_GetButtonCaps(
                        (HIDP_REPORT_TYPE) 0,                       // input
                        buttons,
                        &bcount,
                        preparsed_data
                    ) )
                throw std::runtime_error("HidP_GetButtonCaps failed");
//            _HIDP_BUTTON_CAPS b = buttons[0];
            myButtonCount = bcount;
        }
        else
        {
            myButtonCount = 0;
        }

        if( myCaps.NumberInputValueCaps > 0 )
        {
            unsigned long vcount = myCaps.NumberInputValueCaps;
            HIDP_VALUE_CAPS vals[ vcount ];
            if( HIDP_STATUS_SUCCESS != HidP_GetValueCaps(
                        (HIDP_REPORT_TYPE) 0,                       // input
                        vals,
                        &vcount,
                        preparsed_data
                    ) )
                throw std::runtime_error("HidP_GetValueCaps failed");
//            HIDP_VALUE_CAPS val = vals[0];
            myValueCount = vcount;
        }
        else
        {
            myValueCount = 0;
        }

        bRet = TRUE;
    }
    catch( std::runtime_error& e )
    {

    }
    if( preparsed_data )
        HidD_FreePreparsedData( preparsed_data );

    return bRet;
}

//void cHID::getCAPS( wxString& usage_name )
//{
//    usage_name = wxString::Format("UP %d, P %d, Ns %d ",
//                                  myCaps.UsagePage, myCaps.Usage, myCaps.NumberLinkCollectionNodes );
//    switch( myCaps.Usage )
//    {
//    case HID_USAGE_GENERIC_POINTER:
//        usage_name += "pointer";
//        break;
//    case HID_USAGE_GENERIC_MOUSE:
//        usage_name += "mouse";
//        break;
//    case HID_USAGE_GENERIC_JOYSTICK:
//        usage_name += "joystick";
//        break;
////#define HID_USAGE_GENERIC_GAMEPAD             ((USAGE) 0x05)
//    case HID_USAGE_GENERIC_KEYBOARD:
//        usage_name += "keyboard";
//        break;
//    default:
//
//        break;
//    }
//}

//void cHID::Read()
//{
// //   unsigned char buf[256] = { 0 };
//    DWORD length = 0;
////    ReadFile(
////        myHandle,
////        buf,
////        250,
////        &length,
////        NULL
////    );
//
//    if( length > 0 )
//    {
////        int dbg = 1;
//    }
//}
}
}

