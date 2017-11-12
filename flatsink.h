/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
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
double angl(double, double);

class FlatSink
{
   //temp variables for various functions
   //mapIndexType *borderMap, *idMap;


  //buffer to contain FADIDs
  int  id;


  ConstIteratorType :: IndexType idx;
  NeighborhoodIteratorType :: RadiusType radius;
  ImageType :: RegionType :: SizeType  size;
  ImageType::Pointer borderImage, tmpImage, idImage;
  ReaderType :: Pointer reader;
  long long numberOfPixels;
  double maskValue;
  NeighborhoodIteratorType *neighIt, *idIt, *borderIt;
  FaceCalculatorType::FaceListType::iterator innerAreaIt;


  bool checkFlatSink(mapIndexType*);
  void sfd();
  void setBorder(mapIndexType*, mapIndexType* );

public:
  void fillSinks();
  ~FlatSink();
  FlatSink();
  FlatSink(ReaderType :: Pointer, double);
  void writeImage(std::string);
};

#endif // FLATSINK_H
