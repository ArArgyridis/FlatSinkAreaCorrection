#include "flatsink.h"

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


struct Comparator {
    const ImageType::IndexType *idx;

    Comparator(ImageType::IndexType* _idx) : idx(_idx){}

    bool operator()(const ImageType::IndexType* r1) const
    { return  ( *r1 == *idx); }
};


bool FlatSink::checkFlatSink() {
    NeighborhoodIteratorType :: IndexType *idx;
    idx = new  NeighborhoodIteratorType :: IndexType (neighIt->GetIndex(4) );
    short *cnt;
    cnt = new short (0);
    //counting zero and negatives
    int *i;
    i = new int(0);

    for ( *i = 0;  *i < 9;  (*i)++ )  {
        if ( *i != 4 ) {
            if (neighIt->GetCenterPixel() <= neighIt->GetPixel(*i) )
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

FlatSink::~FlatSink() {

    delete[] idBuf;
    delete[] borderBuf;
    delete neighIt;

}

FlatSink::FlatSink() {

}

FlatSink::FlatSink(ReaderType :: Pointer pnt): reader(pnt) {

    reader->Update();

    numberOfPixels = reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() ;

    idBuf = new int [numberOfPixels];
    memset ( idBuf, 0, sizeof(int)*numberOfPixels );
    id = 1;
    borderBuf = new int [numberOfPixels];
    memset ( borderBuf, 0, sizeof(int)*numberOfPixels );

    radius.Fill(1);

    size = reader->GetOutput()->GetLargestPossibleRegion().GetSize();

    //reading image into tmpImage
    tmpImage = ImageType::New();
    tmpImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    tmpImage->SetSpacing ( reader->GetOutput()->GetSpacing() );
    tmpImage->SetOrigin ( reader->GetOutput()->GetOrigin() );
    tmpImage->Allocate();

    ConstIteratorType inputIt(reader->GetOutput(), reader->GetOutput()->GetLargestPossibleRegion() );
    IteratorType      outputIt(tmpImage,         tmpImage->GetLargestPossibleRegion() );
    for (inputIt.GoToBegin(), outputIt.GoToBegin(); !inputIt.IsAtEnd(); ++inputIt, ++outputIt) {
        outputIt.Set(inputIt.Get());
    }

    neighIt = new  NeighborhoodIteratorType( radius, tmpImage, tmpImage->GetLargestPossibleRegion()  );

}

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

                    //finding maximum slope - SFD
                    PixelType flowHeight = 9999;
                    float maxSlope = 0.0;
                    for (register int pos = 0; pos < 9; pos++) {

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

                    std::vector<PixelType> finalHeights; //vector to store the final heights
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
                                            finalHeights.push_back( (outD*outHeight + inD*inHeight) / (outD + inD) );
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
                        std :: sort ( finalHeights.begin(), finalHeights.end() );
                        localNeighIt.SetCenterPixel( finalHeights[0] ); // perform the linear interpolation

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



void FlatSink::setBorder( mapIndexType *borderMap, mapIndexType *idMap ) {
    NeighborhoodIteratorType :: IndexType idx = neighIt->GetIndex(4);
    idMap->at(idBuf[idx[1]*size[0] +idx[0]  ]).push_back(idx);
    for (register int i = 0; i < 9; i++) {
        if ( i != 4  ) {
            NeighborhoodIteratorType :: IndexType tmpIdx =  neighIt->GetIndex(i);
            if  ( ( idBuf [tmpIdx[1]*size[0] +tmpIdx[0]  ] < 1) ) { //0 is required for the border pixels
                borderBuf[ tmpIdx[1]*size[0] +tmpIdx[0] ] = idBuf[ idx[1]*size[0] +idx[0] ];

                borderMap->at(idBuf[idx[1]*size[0] +idx[0]  ]).push_back( tmpIdx );

            }
        }
    }
}

void FlatSink::writeImage( std::string fileName ) {
    WriterType :: Pointer   writer = WriterType :: New();
    writer->SetFileName(fileName);
    writer->SetInput(tmpImage);
    writer->Update();

}
