#ifndef FLATSINK_H
#define FLATSINK_H
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

#include <itkImageRegionIterator.h>
#include <itkConstNeighborhoodIterator.h>
#include <itkNeighborhoodIterator.h>
#include <otbImage.h>
#include <otbImageFileReader.h>
#include <otbImageFileWriter.h>
#include <otbImportImageFilter.h>


typedef double PixelType;
typedef otb :: Image<PixelType, 2> ImageType;
typedef ImageType :: IndexType IndexType;
typedef otb :: ImageFileReader<ImageType> ReaderType;
typedef otb :: ImageFileWriter<ImageType> WriterType;
typedef itk :: ImageRegionIterator <ImageType> IteratorType;
typedef itk :: ImageRegionConstIterator < ImageType > ConstIteratorType;
typedef itk::NeighborhoodIterator<ImageType> NeighborhoodIteratorType;
typedef itk :: ImageRegionIterator < ImageType >  IteratorType;

typedef  std::map <int, std :: vector<ConstIteratorType :: IndexType> >  mapIndexType;


class FlatSink
{
    //buffer to contain FADIDs
    int id;
    int  *idBuf, *borderBuf;

    ConstIteratorType :: IndexType idx;
    NeighborhoodIteratorType :: RadiusType radius;
    ImageType :: RegionType :: SizeType  size;
    ImageType::Pointer tmpImage;
    ReaderType :: Pointer reader;
    long long numberOfPixels;
    NeighborhoodIteratorType *neighIt;

    NeighborhoodIteratorType :: IndexType __idx, tmpIdx;

    bool checkFlatSink();
    void sfd();
    void setBorder(mapIndexType*, mapIndexType* );

public:
    void fillSinks();
    ~FlatSink();
    FlatSink();
    FlatSink(ReaderType :: Pointer);
    void writeImage(std::string);
};

#endif // FLATSINK_H
