// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: depthdata.h 6bf84c771de0 2012/06/07 13:05:09 Oliver Lau <oliver@von-und-fuer-lau.de> $

#ifndef __DEPTHDATA_H_
#define __DEPTHDATA_H_

#include <QVector>
#include <QSize>
#include "nui.h"

class DepthData
{
public:
    typedef QVector<int> DepthDataType;
    static const int DEPTH_RESOLUTION = 256;

    DepthData(void)
    { /* ... */ }
    DepthData(const DepthData& other)
        : mData(other.mData)
        , mNuiData(other.mNuiData)
        , mSize(other.mSize)
    { /* ... */ }
    DepthData(const DepthDataType& data, const QSize& size, int nearClipping, int farClipping)
        : mData(data)
        , mSize(size)
        , mNearClipping(nearClipping)
        , mFarClipping(farClipping)
    { /* ... */ }
    DepthData(const QSize& size, int nearClipping, int farClipping);
    DepthData(const quint16* nuiData, const QSize& size, int nearClipping, int farClipping);

    const QSize& size(void) const { return mSize; }
    DepthDataType& data(void) { return mData; }
    const DepthDataType& data(void) const { return mData; }
    bool isValid(void) const { return mSize.isValid() && !mData.isEmpty(); }
    DepthData scaled(const QSize& requestedSize);

    void setClipping(int, int);
    void setNearClipping(int);
    void setFarClipping(int);

private:
    QSize mSize;
    DepthDataType mData;
    DepthDataType mNuiData;
    int mNearClipping;
    int mFarClipping;

    void clip(void);
    void setNuiDepthData(const quint16* nuiData, const QSize& size);
    void scaleLine(DepthDataType::iterator dst, DepthDataType::const_iterator src, int srcWidth, int dstWidth);

};

#endif // __DEPTHDATA_H_
