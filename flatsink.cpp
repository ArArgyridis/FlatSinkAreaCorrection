#include "flatsink.h"
bool compareIndex ( NeighborhoodIteratorType :: IndexType a, NeighborhoodIteratorType :: IndexType b ) { //not perfect but works to eliminate duplicate indices from vector
    if ( ( a[0] <= b[0 ] ) && (a[1] <= b[1])  )
        return true;
    else if ( ( a[0] > b[0]) && (a[1] < b[1]) )
        return true;


}



template <class T>
void writeImageFromBuffer(ReaderType :: Pointer reader, T* localBuffer, char* fileName, bool flag  ) {

    typedef otb :: Image<T, 2> ImageOutType;
    typedef otb :: ImageFileWriter<ImageOutType> WriterOutType;
    typedef otb::ImportImageFilter<ImageOutType> ImportFilterType;

    typename WriterOutType :: Pointer   writer = WriterOutType :: New();
    typename ImportFilterType::Pointer importFilter = ImportFilterType :: New();

    writer->SetFileName(fileName);


    int  numberOfPixels = reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() ;
    importFilter->SetRegion(  reader->GetOutput()->GetLargestPossibleRegion() );
    importFilter->SetOrigin( reader->GetOutput()->GetOrigin() );
    importFilter->SetSpacing( reader->GetOutput()->GetSpacing() );
    importFilter->SetImportPointer(localBuffer, numberOfPixels, flag );


    writer->SetInput( dynamic_cast<ImageOutType*>( importFilter->GetOutput() ) );
    writer->Update();
}

int counter = 0;
bool FlatSink :: checkFlatSink( mapIndexType *idMap ) {
    NeighborhoodIteratorType :: IndexType *idx;
    idx =  new  NeighborhoodIteratorType :: IndexType (neighIt->GetIndex(4) );
    short *cnt;
    cnt = new short (0);
    int *i;
    i = new int;

    for ( *i = 0;  *i < 9;  (*i)++ )  {
        if ( *i != 4 ) {
            if ( neighIt->GetCenterPixel() <= neighIt->GetPixel( *i ) )
                (*cnt)++;
        }
    }
    if (*cnt == 8) {//FS pixel
        idIt->SetLocation ( neighIt->GetIndex() );
        idIt->SetCenterPixel( id );
        idMap->at( id ).push_back( neighIt->GetIndex() );

        for ( *i = 0;  *i < 9;  (*i) ++ )  {
            if (*i != 4) {
                idIt->SetLocation (  neighIt->GetIndex( *i ) ); //going to check if each id in the 3x3 area is set
                if ( ( idIt->GetCenterPixel() == 0  ) && (neighIt->GetCenterPixel() != 0 ) ) {
                    neighIt->SetLocation(  neighIt->GetIndex( *i )  );
                    checkFlatSink( idMap );
                    neighIt->SetLocation(*idx);
                    idIt->SetLocation(*idx);
                }
            }
        }
        delete i;
        delete cnt;
        delete idx;
        return true;

    }
    else {
        idIt->SetLocation(*idx );
        idIt->SetCenterPixel( -1 );
        delete idx;
        return false;
    }
}


/*
bool FlatSink::checkFlatSink() {
    NeighborhoodIteratorType :: IndexType *idx;
    idx = new  NeighborhoodIteratorType :: IndexType (neighIt->GetIndex(4) );
    short *cnt;
    cnt = new short (0);
    //counting zero and negatives
    int *i;
    i = new int;

    for ( *i = 0;  *i < 9;  (*i)++ )  {
        if ( *i != 4 ) {
            if ( neighIt->GetCenterPixel() <= neighIt->GetPixel( *i ) )
                (*cnt)++;
        }
    }

    if (*cnt == 8) {//FS pixel
        idBuf [  (*idx)[1]*size[0] + (*idx)[0]  ] = id;
        for ( *i = 0;  *i < 9;  (*i) ++ )  {

            if (*i != 4) {
                NeighborhoodIteratorType :: IndexType *tmpIdx;
                tmpIdx =  new NeighborhoodIteratorType :: IndexType (neighIt->GetIndex( *i ) );

                if  ( ( (*tmpIdx)[0] > 0 ) && ((*tmpIdx)[1] > 0 ) && ( (*tmpIdx)[0] < size[0]-1 ) && ((*tmpIdx)[1] < size[1]-1 )  && ( idBuf [(*tmpIdx)[1]*size[0] +(*tmpIdx)[0]  ] == 0 ) ) {
                    neighIt->SetLocation( (*tmpIdx) );
                    //recursing until all nearby FS pixels are detected
                    checkFlatSink();
                    neighIt->SetLocation( *idx );
                }
                delete tmpIdx;
            }
        }
        delete i;
        delete idx;
        delete cnt;
        return true;
    }
    else {
        idBuf [  (*idx)[1]*size[0] + (*idx)[0]  ] = -1;
        delete idx;
        return false;
    }
}
*/

FlatSink::~FlatSink() {

    delete[] idBuf;
    delete[] borderBuf;
    delete neighIt;
    delete idIt;

}

FlatSink::FlatSink() {

}

FlatSink::FlatSink(ReaderType :: Pointer pnt): reader(pnt) {

    reader->Update();
    id = 1;
    radius.Fill(1);
    size = reader->GetOutput()->GetLargestPossibleRegion().GetSize();


    numberOfPixels = reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() ;

    idBuf = new int [numberOfPixels];
    memset ( idBuf, 0, sizeof(int)*numberOfPixels );

    borderBuf = new int [numberOfPixels];
    memset ( borderBuf, 0, sizeof(int)*numberOfPixels );


    //reading image into tmpImage
    tmpImage = ImageType :: New();
    tmpImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    tmpImage->CopyInformation(reader->GetOutput());
    tmpImage->Allocate();

    ConstIteratorType inputIt(reader->GetOutput(), reader->GetOutput()->GetLargestPossibleRegion() );
    IteratorType      outputIt(tmpImage,         tmpImage->GetLargestPossibleRegion() );
    for (inputIt.GoToBegin(), outputIt.GoToBegin(); !inputIt.IsAtEnd(); ++inputIt, ++outputIt) {
        outputIt.Set(inputIt.Get());
    }



    //creating image to store the ids
    idImage = ImageType :: New();
    idImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    idImage->CopyInformation(reader->GetOutput());
    idImage->Allocate();

    //extracting the inner region of the image
    FaceCalculatorType faceCalculator;
    FaceCalculatorType::FaceListType faceList;

    faceList = faceCalculator(idImage, idImage->GetRequestedRegion(), radius);


    innerAreaIt = faceList.begin();
    IteratorType idIt;
    idIt = IteratorType(idImage, *innerAreaIt);
    for (idIt.GoToBegin(); !idIt.IsAtEnd(); ++idIt)
        idIt.Set( 0 );




    for (++innerAreaIt;  innerAreaIt !=faceList.end(); ++innerAreaIt) {
        idIt = IteratorType(idImage, *innerAreaIt);

        for (idIt.GoToBegin(); !idIt.IsAtEnd(); ++idIt)
            idIt.Set( -1 );

    }



    innerAreaIt = faceList.begin();

    neighIt = new  NeighborhoodIteratorType( radius, tmpImage, *innerAreaIt );
    this->idIt = new NeighborhoodIteratorType( radius, idImage, *innerAreaIt );

}


void FlatSink::fillSinks() {
    bool recompute = false;

    std::cout <<"heree\n";
    mapIndexType *borderMap, *idMap;
    idMap = new mapIndexType;
    borderMap = new mapIndexType;

    std :: vector<ConstIteratorType :: IndexType> k;
    (*idMap)[id] = k;

    for ( idIt->GoToBegin(), neighIt->GoToBegin(); !idIt->IsAtEnd(); neighIt->operator ++(), idIt->operator ++() ) {
        if ( (idIt->GetCenterPixel() == 0) && (neighIt->GetCenterPixel() != 0 ) )
            if ( checkFlatSink( idMap ) ) {// when the if is entered all flat/sink pixels beloning to the same id are determined
                (*borderMap)[id] = k;
                setBorder(borderMap, idMap); //getting the border of each FAD
                id++;
                (*idMap)[id] = k;
            }


    }
    /*
    WriterType :: Pointer kkk = WriterType :: New();
    kkk->SetFileName( "checkids.img" );
    kkk->SetInput(idImage);
    kkk->Update();
    */

    std :: cout << idMap->size() << "\n";
    delete idMap;
    delete borderMap;
}



/*
void FlatSink::fillSinks() {
  bool recompute = false;
  ImageType::Pointer localImage;
  localImage = ImageType :: New();
  localImage->SetRegions( tmpImage->GetLargestPossibleRegion() );
  localImage->SetSpacing ( tmpImage->GetSpacing() );
  localImage->SetOrigin ( tmpImage->GetOrigin() );
  localImage->Allocate();

  IteratorType tmpIt(tmpImage, tmpImage->GetLargestPossibleRegion() );
  IteratorType      localIt(localImage,         localImage->GetLargestPossibleRegion() );

  for (localIt.GoToBegin(), tmpIt.GoToBegin(); !tmpIt.IsAtEnd(); ++localIt, ++tmpIt) {
      localIt.Set(tmpIt.Get());
    }

  do {

      NeighborhoodIteratorType  localNeighIt  ( radius, localImage, localImage->GetLargestPossibleRegion()  );

      recompute = false;
      id = 1;
      for (neighIt->GoToBegin(); !neighIt->IsAtEnd(); neighIt->operator ++()  ) {
          idx = neighIt->GetIndex();

          if  ( ( idx[0] > 0 ) && ( idx[1] > 0 ) && ( idx[0] < size[0]-1 ) && ( idx[1] < size[1]-1 )  && ( idBuf [ idx[1]*size[0] +idx[0]  ] == 0 ) ) {

              bool check = checkFlatSink();
              if (check)
                id++;
            }
        }

      //getting the borders of the areas
      mapIndexType borderMap, idMap;


      //initializing border map

      for (register int i = 1; i < id; i++) {
          std :: vector<ConstIteratorType :: IndexType> vec, vec2;
          borderMap[i] =  vec;
          idMap[i] = vec2;
        }

      for ( neighIt->GoToBegin(); !neighIt->IsAtEnd(); neighIt->operator ++() ) {
          idx = neighIt->GetIndex();
          if  (  idBuf [ idx[1]*size[0] +idx[0]  ] > 0 ) {
              setBorder(&borderMap, &idMap);
            }
        }

      //writeImageFromBuffer(reader, idBuf, "checkids.tif", false);
      //writeImageFromBuffer(reader, borderBuf, "checkborder.tif", false);

      mapIndexType::iterator itMap, itBor;

      std :: cout <<"Number of detected flat-sink areas: " << idMap.size() <<"\n";
      for( itMap = idMap.begin(), itBor=borderMap.begin(); itMap != idMap.end(); itMap++, itBor++) {//for each FAD
          //find minimum and maximum border heights
          PixelType minHeight, maxHeight;
          neighIt->SetLocation((*itBor).second[0]);
          minHeight = maxHeight = neighIt->GetCenterPixel();

          for (register int i = 1; i < (*itBor).second.size(); i++ ) {
              neighIt->SetLocation((*itBor).second[i]);
              PixelType tmp = neighIt->GetCenterPixel();
              if (minHeight > tmp)
                minHeight = tmp;

              if (maxHeight < tmp)
                maxHeight = tmp;
            }

          //if min and max heights of the border are the same - SPECIAL CASE 1
          if (minHeight == maxHeight) {
              for (register int i = 0; i < (*itMap).second.size(); i++) {
                  localNeighIt.SetLocation((*itMap).second[i]);
                  localNeighIt.SetCenterPixel(minHeight);
                }

              //correcting the height of border pixels. This is done by using the height of the pixel in which each border pixel flows into
              for(register int i = 0; i < (*itBor).second.size(); i++ ) {
                  neighIt->SetLocation((*itBor).second[i]);

                  //finding maximum slope - SFDidMap->at(id).begin()
                  PixelType flowHeight = 9999;
                  float maxSlope = 0.0;
                  for (register int pos = 0; pos < 9; pos++) {
borderMap
                      ImageType::IndexType tmpIdx = (*itBor).second[i] + neighIt->GetOffset(pos);

                      if  ( ( idBuf [ tmpIdx[1]*size[0] +tmpIdx[0]  ] < 1) && ( ( borderBuf [ tmpIdx[1]*size[0] +tmpIdx[0]  ] == 0 ) ) ) {//the examined pixel is neither grouped nor border

                          float tmpSlope = ( neighIt->GetCenterPixel() - neighIt->GetPixel(pos) )/std :: sqrt ( std :: pow ( neighIt->GetOffset(pos)[0],2  ) + std :: pow (neighIt->GetOffset(pos)[1] , 2)  ) ;

                          if ( maxSlope < tmpSlope   ) {
                              maxSlope = tmpSlope;
                              flowHeight = neighIt->GetPixel(pos);
                            }
                        }
                    }
                  if (maxSlope > 0.0 ) {
                      localNeighIt.SetLocation( (*itBor).second[i] );
                      localNeighIt.SetCenterPixel( 0.9*neighIt->GetCenterPixel() + 0.1*flowHeight );
                    }
                }

            } else {//Trying to perform linear interpolation

              //finding outlet/inflow pixels
              std :: vector <ConstIteratorType :: IndexType*> outlet, inflow;

              for (register int i = 0; i < (*itBor).second.size(); i++) {
                  neighIt->SetLocation((*itBor).second[i] );
                  if (neighIt->GetCenterPixel() == minHeight)
                    outlet.push_back( &(*itBor).second[i] );
                  else
                    inflow.push_back( &(*itBor).second[i] );
                }

              //for each fs pixel
              int unprocessedFSCounter = 0;
              for ( std :: vector<ConstIteratorType :: IndexType>:: iterator fs = (*itMap).second.begin(); fs != (*itMap).second.end(); fs++ ) {

                  PixelType finalHeights = std :: pow(10, 6); // final heights
                  //computing maximum distance of the inflow pixels from the examined FS pixel
                  int maxD = 10 * std :: sqrt (std ::  pow ( (*fs)[1] - (*inflow[0])[1] , 2 ) +  std :: pow ( (*fs)[0] - (*inflow[0])[0] , 2) ) + 1;
                  for (register int i = 1; i < inflow.size(); i++) {
                      int dd = 10 * std :: sqrt (std ::  pow ( (*fs)[1] - (*inflow[i])[1] , 2 ) +  std :: pow ( (*fs)[0] - (*inflow[i])[0] , 2) ) + 1;
                      if (maxD < dd)
                        maxD = dd;
                    }

                  int outletToOutletCounter = 0;

                  for ( register int i = 0; i < outlet.size(); i++ ) {

                      double outHeight, outD;
                      double ang = std :: atan2( (*fs)[1] - (*outlet[i])[1],  (*fs)[0] - (*outlet[i])[0] );
                      outD = std :: sqrt (std ::  pow ( (*fs)[1] - (*outlet[i])[1] , 2 ) +  std :: pow ( (*fs)[0] - (*outlet[i])[0] , 2) );
                      int d = 10*outD + 1;


                      //determining if the OF line is inside the FAD
                      bool cont2fs = true;

                      //checking if the line from the outlet towards the pixel is completely inside the FAD
                      for ( register int m = 1;  ( (m <= d)  && (cont2fs == true) ); m++ ) {
                          ImageType::IndexType index;
                          index[0] = round ( (*outlet[i])[0] + m*0.1*cos(ang) );
                          index[1] = round ( (*outlet[i])[1] + m*0.1*sin(ang) );

                          if  ( (index ==  (*outlet[i])  ) || ( idBuf [ index[1]*size[0] +index[0] ] == (*itBor).first  )  ) {//the line is passing through the outlet or a pixel belonging to the group
                              if (index == *fs) { //if I reach the examined flat sink pixel
                                  cont2fs = false;
                                  neighIt->SetLocation(*outlet[i]);
                                  outHeight = neighIt->GetCenterPixel();
                                  //continue with extending the line from the fs towards the border

                                  bool cont2border = true;
                                  for (register int m = 1;  ( (m <= maxD)  && (cont2border == true) ); m++) {
                                      index[0] = round ( (*fs)[0] + m*0.1*cos(ang) );
                                      index[1] = round ( (*fs)[1] + m*0.1* sin(ang) );
                                      neighIt->SetLocation( index );
                                      //checking if the pixel is inflow or outlet
                                      if ( ( idBuf [ index[1]*size[0] +index[0] ] < 1 ) && ( neighIt->GetCenterPixel() > minHeight ) ) { //condition for boundary and inflow pixel
                                          double inHeight = neighIt->GetCenterPixel();
                                          double inD = std :: sqrt (std ::  pow ( (*fs)[1] - index[1] , 2 ) +  std :: pow ( (*fs)[0] - index[0] , 2) );
                                          //std :: cout << (*itMap).first << "\t" << index  << "\t" << (outD*outHeight + inD*inHeight) / (outD + inD)  << "\t" << (*itBor).second.size() <<  "\n";
                                          PixelType tmpH = (outD*outHeight + inD*inHeight) / (outD + inD);
                                          if (finalHeights > tmpH)
                                            finalHeights = tmpH;
                                          cont2border = false;
                                        }
                                      else if ( (  idBuf [ index[1]*size[0] +index[0] ] < 1 ) && ( neighIt->GetCenterPixel() == minHeight ) ) { //condition for boundary and outlet pixel - SPECIAL CASE 2
                                          outletToOutletCounter++;
                                        }

                                    }
                                }
                            }
                          else { //in case when the line falls out of the FAD or the examined outlet
                              cont2fs = false;
                            }
                        }
                    }

                  if ( outletToOutletCounter == outlet.size() )
                    unprocessedFSCounter++;
                  else {
                      localNeighIt.SetLocation(*fs);
                      localNeighIt.SetCenterPixel( finalHeights ); // perform the linear interpolation

                    }
                }
              if ( unprocessedFSCounter == (*itMap).second.size()   ) {//all fs pixels remained unprocessed since special case 2 applied
                  //recompute heights for all border pixels
                  for(register int i = 0; i < (*itBor).second.size(); i++ ) {
                      neighIt->SetLocation((*itBor).second[i]);

                      //finding maximum slope - SFD
                      PixelType flowHeight = 9999;
                      float maxSlope = 0.0;
                      for (register int pos = 0; pos < 9; pos++) {

                          ImageType::IndexType tmpIdx = (*itBor).second[i] + neighIt->GetOffset(pos);

                          if  (  ( idBuf [ tmpIdx[1]*size[0] +tmpIdx[0]  ] < 1) && ( ( borderBuf [ tmpIdx[1]*size[0] +tmpIdx[0]  ] == 0 ) ) ) {//the examined pixel is neither grouped nor border

                              float tmpSlope = ( neighIt->GetCenterPixel() - neighIt->GetPixel(pos) )/std :: sqrt ( std :: pow ( neighIt->GetOffset(pos)[0],2  ) + std :: pow (neighIt->GetOffset(pos)[1] , 2)  ) ;

                              if ( maxSlope < tmpSlope   ) {
                                  maxSlope = tmpSlope;
                                  flowHeight = neighIt->GetPixel(pos);
                                }
                            }
                        }
                      if (maxSlope > 0.0 ) {
                          localNeighIt.SetLocation( (*itBor).second[i] );
                          localNeighIt.SetCenterPixel( 0.9*neighIt->GetCenterPixel() + 0.1*flowHeight );

                        }
                    }
                }
            }
        }
      if (idMap.size() > 0) {
          recompute = true;
          memset ( idBuf, 0, sizeof(int)*numberOfPixels );
          memset ( borderBuf, 0, sizeof(int)*numberOfPixels );

        }
      borderMap.clear();
      idMap.clear();
      //copy local image to tmpImage
      for (localIt.GoToBegin(), tmpIt.GoToBegin(); !tmpIt.IsAtEnd(); ++localIt, ++tmpIt) {
          tmpIt.Set(localIt.Get());
        }
    }while (recompute);
}
*/


void FlatSink :: setBorder ( mapIndexType *borderMap, mapIndexType *idMap )  {
    NeighborhoodIteratorType :: IndexType tmpIdx = idIt->GetIndex();

    for ( std :: vector<ConstIteratorType :: IndexType> :: iterator it = idMap->at(id).begin();    it != idMap->at(id).end(); it++  ) {
        idIt->SetLocation( *it );
        for (register int i = 0; i < 9; i++) {
            if ( ( i != 4  ) && (  idIt->GetPixel( i ) < 1 ) )
               // if  ( std::find(borderMap->at(id).begin(), borderMap->at(id).end(), idIt->GetIndex(i)  ) != borderMap->at(id).end()  )
                    borderMap->at(id).push_back( idIt->GetIndex(i) );
        }
    }


    std :: sort_heap ( borderMap->at(id).begin(), borderMap->at(id).end(), compareIndex );
    /*
    for (register int i = 0; i < borderMap->at(id).size(); i++)
        std::cout << borderMap->at(id)[i] <<"\t";
    std::cout <<"\n";
    */
     borderMap->at(id).erase( std:: unique( borderMap->at(id).begin(), borderMap->at(id).end() ), borderMap->at(id).end() );

    /*

    for (register int i = 0; i < borderMap->at(id).size(); i++)
        std::cout << borderMap->at(id)[i] <<"\t";
    std::cout <<"\n";
    */
    idIt->SetLocation( tmpIdx );
}


/*

void FlatSink::setBorder( mapIndexType *borderMap, mapIndexType *idMap ) {

    idMap->at(  idIt->GetCenterPixel() ).push_back( idIt->GetIndex() );
    for (register int i = 0; i < 9; i++) {
        if ( i != 4  ) {
            NeighborhoodIteratorType :: IndexType tmpIdx =  neighIt->GetIndex(i);
            if  ( ( idBuf [tmpIdx[1]*size[0] +tmpIdx[0]  ] < 1) ) {
                borderBuf[ tmpIdx[1]*size[0] +tmpIdx[0] ] = idBuf[ idx[1]*size[0] +idx[0] ];

                borderMap->at(  idIt->GetCenterPixel()  ).push_back( tmpIdx );

            }
        }
    }
}
*/

void FlatSink::writeImage( std::string fileName ) {
    WriterType :: Pointer   writer = WriterType :: New();
    writer->SetFileName(fileName);
    writer->SetInput(tmpImage);
    writer->Update();

}

