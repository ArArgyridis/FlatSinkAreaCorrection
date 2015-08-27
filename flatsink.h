#ifndef FLATSINK_H
#define FLATSINK_H
#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>

#include <itkImageRegionIterator.h>
#include <itkConstNeighborhoodIterator.h>
#include "itkNeighborhoodAlgorithm.h"
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

typedef itk::NeighborhoodAlgorithm :: ImageBoundaryFacesCalculator<ImageType> FaceCalculatorType;

typedef  std::map <int, std :: vector<ConstIteratorType :: IndexType> >  mapIndexType;

bool compareIndex(ConstIteratorType :: IndexType, ConstIteratorType :: IndexType);

class FlatSink
{
   //temp variables for various functions
   //mapIndexType *borderMap, *idMap;


  //buffer to contain FADIDs
  int id;
  int  *idBuf, *borderBuf;

  ConstIteratorType :: IndexType idx;
  NeighborhoodIteratorType :: RadiusType radius;
  ImageType :: RegionType :: SizeType  size;
  ImageType::Pointer tmpImage, idImage;
  ReaderType :: Pointer reader;
  long long numberOfPixels;
  NeighborhoodIteratorType *neighIt, *idIt;
  FaceCalculatorType::FaceListType::iterator innerAreaIt;


  bool checkFlatSink(mapIndexType*);
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
