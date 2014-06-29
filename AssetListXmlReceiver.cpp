#include <stdexcept>
#include <typeinfo>

#include <QCoreApplication>

#include "AssetListXmlReceiver.h"

namespace Evernus
{
    template<>
    void APIXmlReceiver<AssetList::value_type, std::unique_ptr<AssetList::value_type::element_type>>::startElement(const QXmlName &name)
    {
        if (name.localName(mNamePool) == "row")
        {
            mCurrentElement.reset(new AssetList::value_type::element_type{});
        }
        else if (name.localName(mNamePool) == "rowset")
        {
            mAdditionalData.mParsingRowset = true;
            mAdditionalData.mElementStack.emplace(std::move(mCurrentElement));
        }
    }

    template<>
    void APIXmlReceiver<AssetList::value_type, std::unique_ptr<AssetList::value_type::element_type>>::endElement()
    {
        if (mCurrentElement)
        {
            if (mAdditionalData.mElementStack.empty())
                mContainer.emplace_back(std::move(mCurrentElement));
            else
                mAdditionalData.mElementStack.top()->addItem(std::move(mCurrentElement));
        }
        else if (mAdditionalData.mParsingRowset)
        {
            Q_ASSERT(!mAdditionalData.mElementStack.empty());
            mContainer.emplace_back(std::move(mAdditionalData.mElementStack.top()));
            mAdditionalData.mElementStack.pop();
            mAdditionalData.mParsingRowset = false;
        }
    }

    template<>
    void APIXmlReceiver<AssetList::value_type, std::unique_ptr<AssetList::value_type::element_type>>::attribute(const QXmlName &name, const QStringRef &value)
    {
        const auto localName = name.localName(mNamePool);
        if (localName == "itemID")
            mCurrentElement->setId(convert<Item::IdType>(value.toString()));
        else if (localName == "typeID")
            mCurrentElement->setTypeId(convert<ItemData::TypeIdType>(value.toString()));
        else if (localName == "locationID")
            mCurrentElement->setLocationId(convert<ItemData::LocationIdType::value_type>(value.toString()));
        else if (localName == "quantity")
            mCurrentElement->setQuantity(convert<uint>(value.toString()));
    }
}
