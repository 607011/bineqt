// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: depthdata.cpp 6bf84c771de0 2012/06/07 13:05:09 Oliver Lau <oliver@von-und-fuer-lau.de> $

#include "depthdata.h"


DepthData::DepthData(const quint16* nuiData, const QSize& size, int nearClipping, int farClipping)
    : mSize(size)
    , mData(DepthDataType(size.width() * size.height()))
    , mNearClipping(nearClipping)
    , mFarClipping(farClipping)
{
    setNuiDepthData(nuiData, size);
}


DepthData::DepthData(const QSize& size, int nearClipping, int farClipping)
    : mSize(size)
    , mData(DepthDataType(size.width() * size.height()))
    , mNearClipping(nearClipping)
    , mFarClipping(farClipping)
{
    /* ... */
}


void DepthData::clip(void)
{
    if (mFarClipping <= mNearClipping)
        return;
    const int DEPTH_RANGE = mFarClipping - mNearClipping;
    DepthDataType::const_iterator src = mNuiData.begin();
    mData.resize(mNuiData.size());
    for (DepthDataType::iterator dst = mData.begin(); dst != mData.end(); ++dst) {
        int depth = *src++;
        if (depth < mNearClipping
                || depth > mFarClipping
                || depth == NUI_IMAGE_DEPTH_NO_VALUE
                || depth == NUI_IMAGE_DEPTH_TOO_FAR_VALUE)
            depth = mFarClipping;
        *dst = DEPTH_RESOLUTION - DEPTH_RESOLUTION * (depth - mNearClipping) / DEPTH_RANGE;
    }
}


void DepthData::setNuiDepthData(const quint16* nuiData, const QSize& size)
{
    if (nuiData == NULL || !size.isValid())
        return;
    int N = size.height() * size.width();
    mNuiData.resize(N);
    mSize = size;
    DepthDataType::iterator i = mNuiData.begin();
    while (N-- > 0)
        *i++ = *nuiData++;
    clip();
}


inline void DepthData::scaleLine(DepthData::DepthDataType::iterator dst, DepthData::DepthDataType::const_iterator src, int srcWidth, int dstWidth)
{
    // grobe Skalierung nach Bresenham (http://www.drdobbs.com/184405045)
    const int stepI = srcWidth / dstWidth;
    const int stepF = srcWidth % dstWidth;
    int E = 0;
    int N = dstWidth;
    while (N-- > 0) {
        *dst++ = *src;
        src += stepI;
        E += stepF;
        if (E >= dstWidth) {
            E -= dstWidth;
            ++src;
        }
    }
}


/***
 * Grobe Skalierung nach Bresenham (http://www.drdobbs.com/184405045)
 */
DepthData DepthData::scaled(const QSize& requestedSize)
{
    if (requestedSize == mSize)
        return *this;
    DepthData scaledDepthData = DepthData(requestedSize, mNearClipping, mFarClipping);
    const int srcHeight = mSize.height();
    const int srcWidth = mSize.width();
    const int dstHeight = requestedSize.height();
    const int dstWidth = requestedSize.width();
    const int stepI = (srcHeight / dstHeight) * srcWidth;
    const int stepF = srcHeight % dstHeight;
    DepthDataType::const_iterator prevSrc = NULL;
    DepthDataType::const_iterator src = mData.begin();
    DepthDataType::iterator dst = scaledDepthData.data().begin();
    int E = 0;
    int N = dstHeight;
    while (N-- > 0) {
        if (src != prevSrc) {
            scaleLine(dst, src, srcWidth, dstWidth);
            prevSrc = src;
        }
        else {
            // vorherige Zeile duplizieren
            memcpy(dst, dst - dstWidth, dstWidth * sizeof(DepthDataType::value_type));
        }
        dst += dstWidth;
        src += stepI;
        E += stepF;
        if (E >= dstHeight) {
            E -= dstHeight;
            src += srcWidth;
        }
    }
    return scaledDepthData;
}


void DepthData::setNearClipping(int nearClipping)
{
    mNearClipping = nearClipping;
    clip();
}


void DepthData::setFarClipping(int farClipping)
{
    mFarClipping = farClipping;
    clip();
}


void DepthData::setClipping(int nearClipping, int farClipping)
{
    mNearClipping = nearClipping;
    mFarClipping = farClipping;
    clip();
}
