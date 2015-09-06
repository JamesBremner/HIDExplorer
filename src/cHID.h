#include <windows.h>
#include <string>
#include <vector>
#include <memory>

#include <setupapi.h>
#include <ddk/hidsdi.h>

namespace raven
{
namespace hid
{
/**

  Details of a HID

*/
class cHID
{
public:

    /**

    Construct cHID

    @param[in] path

    Attemps to open the HID with the path,
    read and store details

    */
    cHID( const char * path );

    /** Ensure we have a good HID

    Sets myStatus flag to false if data gathered has problems

    */
    void SanityTest();

    const std::wstring& getManufacturer()
    {
        return myManufacturer;
    }
    const std::wstring& getDescription()
    {
        return myDescription;
    }
    const std::wstring& getProduct()
    {
        return myProduct;
    }
    int getVendorID()
    {
        return myVendorID;
    }
    int getProductID()
    {
        return myProductID;
    }
    int getButtonCount()
    {
        return myButtonCount;
    }
    int getUsagePage()
    {
        return myCaps.UsagePage;
    }
    int getUsage()
    {
        return myCaps.Usage;
    }
    int getNodes()
    {
        return myCaps.NumberLinkCollectionNodes;
    }
    const std::string & getPath()
    {
        return myPath;
    }
    int getValueCount()
    {
        return myValueCount;
    }
    bool getStatus() { return myStatus; }
    //void getCAPS( std::wstring& usage_name );
    //void Read();

private:
    int myIndex;
    bool myStatus;
    std::string myPath;
    int myVendorID;
    int myProductID;
    std::wstring myManufacturer;
    std::wstring myProduct;
    std::wstring myPhysical;
    std::wstring myDescription;

    int myButtonCount;
    int myValueCount;

    _HIDP_CAPS myCaps;

    bool readCAPS( HANDLE myHandle );

};

/**

 A collection of HID handles

*/
class cVectorHID
{
public:

    /// the collection
    std::vector< std::shared_ptr< cHID > > myHID;

    // enable iteration over the collection
    typedef std::vector< std::shared_ptr< cHID > >::iterator iterator;
    iterator begin()
    {
        return myHID.begin();
    }
    iterator end()
    {
        return myHID.end();
    }

    /**

    Populate collection with all installed HIDs

    */
    void Populate();

    /** Const members of collections

    @return number of HIDs collected

    */
    int size()
    {
        return (int) myHID.size();
    }

    void FilterOutNonCompliant();
};

/**

  Human readable HID collection

*/
class cDumpHID
{
public:
    typedef std::vector< std::vector< std::wstring > > dump_t;
    dump_t myDump;

    /**

    Construct human readable HID collection

    @param[in] vHID  HID collection

    */
    cDumpHID( cVectorHID & vHID );

    /**

    Human readable text string describing HID collection.

    @param[out] text suitable for output to console, one line per HID in CSV format

    */
    void Text( std::wstring& text );
};
}
}

