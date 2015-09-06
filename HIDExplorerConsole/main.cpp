#include <iostream>
#include "cHID.h"

using namespace std;

int main()
{
    cout << "Raven's Point HID explorer" << endl;

    // construct empty HID collection
    raven::hid::cVectorHID vHID;

    // populate HID collection with all installed HIDs
    vHID.Populate();

    raven::hid::cVectorHID vHIDOK( vHID );
    vHIDOK.FilterOutNonCompliant();

    // construct human readable dump of HID collection
    raven::hid::cDumpHID dump( vHIDOK );

    //display HID details
    wstring s;
    cout << "\nFully compliant HIDs\n";
    dump.Text( s );
    wcout << s;


   // construct human readable dump of HID collection
    raven::hid::cDumpHID dumpAll( vHID );

    //display HID details
    cout << "\nFully + Partially Compliant HIDs\n";
    dumpAll.Text( s );
    wcout << s;


    return 0;
}
