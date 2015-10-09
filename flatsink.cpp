#include "flatsink.h"
#include <stdlib.h>
#include <stdio.h>

int cols;
int rows;
bool compareIndex ( NeighborhoodIteratorType :: IndexType a, NeighborhoodIteratorType :: IndexType b ) { //not perfect but works to eliminate duplicate indices from vector
    return !( a[1]*cols + a[0] < b[1]*cols + b[0] );
}

double angl (double a,double b) {
    if ( ( a >=0 ) && ( b >0 ) )
        return std :: atan(a/b);
    else if  ( ( a > 0 ) && ( b < 0) )
        return M_PI - std :: abs( atan(a/b) ) ;
    else if  ( (a < 0 ) && ( b < 0 ) )
        return M_PI + std :: abs( atan (a/b) ) ;
    else if ( ( a < 0 ) && ( b > 0) )
        return 2*M_PI - std :: abs (atan( a/b  ) );
    else if ( (a > 0) && ( b == 0) )
        return M_PI;
    else if ( ( a == 0) && (b < 0) )
        return M_PI;
    else if ( (a < 0 ) && (b == 0))
        return 3.0/2.0*M_PI;
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

bool FlatSink :: checkFlatSink( mapIndexType *idMap ) {
    NeighborhoodIteratorType :: IndexType *idx;
    idx =  new  NeighborhoodIteratorType :: IndexType (neighIt->GetIndex() );
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
        idIt->SetLocation ( *idx );
        idIt->SetCenterPixel( id );
        idMap->at( id ).push_back( *idx );

        for ( *i = 0;  *i < 9;  (*i) ++ )  {
            if (*i != 4) {
                if ( ( idIt->GetPixel( *i ) == 0  ) && (neighIt->GetPixel (*i) != maskValue ) ) {
                    neighIt->SetLocation(  neighIt->GetIndex( *i )  );
                    checkFlatSink( idMap );
                    neighIt->SetLocation( *idx );
                    idIt->SetLocation( *idx );
                }
            }
        }

        delete i;
        i = NULL;
        delete cnt;
        cnt = NULL;
        delete idx;
        idx = NULL;
        return true;

    }
    else {
        idIt->SetLocation(*idx );
        idIt->SetCenterPixel( -1 );
        delete i;
        i = NULL;
        delete cnt;
        cnt = NULL;
        delete idx;
        idx = NULL;

        return false;
    }
}



FlatSink::~FlatSink() {


    delete neighIt;
    delete idIt;

}

FlatSink::FlatSink() {

}

FlatSink::FlatSink(ReaderType :: Pointer pnt, double val): reader(pnt), maskValue(val) {

    reader->Update();
    id = 1;
    radius.Fill(1);
    size = reader->GetOutput()->GetLargestPossibleRegion().GetSize();


    numberOfPixels = reader->GetOutput()->GetLargestPossibleRegion().GetNumberOfPixels() ;

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
    ImageType::RegionType region ;

    //creating image to store the ids
    idImage = ImageType :: New();
    idImage->SetRegions( reader->GetOutput()->GetLargestPossibleRegion() );
    idImage->CopyInformation(reader->GetOutput());
    idImage->Allocate();
    //extracting the inner region of the image
    FaceCalculatorType faceCalculator;
    FaceCalculatorType::FaceListType faceList;

    NeighborhoodIteratorType :: RadiusType radiusFace;
    radiusFace.Fill(2);
    faceList = faceCalculator(idImage, idImage->GetRequestedRegion(), radiusFace);

    //setting the inner area of the id image to 0
    innerAreaIt = faceList.begin();
    IteratorType idIt;
    idIt = IteratorType(idImage, *innerAreaIt);
    for (idIt.GoToBegin(); !idIt.IsAtEnd(); ++idIt)
        idIt.Set( 0 );

    //setting the frame of the id image to -1 thus to be ignored by the algorithm
    for (++innerAreaIt;  innerAreaIt !=faceList.end(); ++innerAreaIt) {
        idIt = IteratorType(idImage, *innerAreaIt);

        for (idIt.GoToBegin(); !idIt.IsAtEnd(); ++idIt)
            idIt.Set( -1 );
    }

    innerAreaIt = faceList.begin();
    rows = size[1];
    cols = size[0];
    neighIt = new  NeighborhoodIteratorType( radius, tmpImage, *innerAreaIt );
    this->idIt = new NeighborhoodIteratorType( radius, idImage, *innerAreaIt );
}


void FlatSink::fillSinks() {
    bool recompute = false;
    int reps = 0;
    do {
        recompute = false;
        mapIndexType *borderMap, *idMap;
        idMap = new mapIndexType;
        borderMap = new mapIndexType;

        std :: vector<ConstIteratorType :: IndexType> k;
        (*idMap)[id] = k;

        for ( idIt->GoToBegin(), neighIt->GoToBegin(); !neighIt->IsAtEnd(); neighIt->operator ++(), idIt->operator ++() ) {
            /*
            if (idIt->GetIndex() != neighIt->GetIndex() )
                std :: cout <<idIt->GetIndex() << "\t" <<neighIt->GetIndex() << "\n";
           */
            if ( (idIt->GetCenterPixel() == 0) && (neighIt->GetCenterPixel() != maskValue ) )
                if ( checkFlatSink( idMap ) ) {// when the if is entered all flat/sink pixels beloning to the same id are determined
                    (*borderMap)[id] = k;
                    setBorder(borderMap, idMap); //getting the border of each FAD
                    id++;
                    (*idMap)[id] = k;
                }

        }
        idMap->erase(id);
        /*
        std :: stringstream ss;
        ss << reps;
        ss  <<".img";
        std :: cout << ss.str() <<"\n";
        WriterType :: Pointer   writer = WriterType :: New();
        writer->SetFileName( ss.str() );
        writer->SetInput(idImage );
        writer->Update();
        */

        std :: cout <<"Number of detected flat-sink areas: " << idMap->size() <<"\n";


        mapIndexType::iterator itMap, itBor;
        for( itMap = idMap->begin(), itBor=borderMap->begin(); itMap != idMap->end(); itMap++, itBor++) {//for each FAD


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
                    neighIt->SetLocation((*itMap).second[i]);
                    neighIt->SetCenterPixel(minHeight);
                }

                //correcting the height of border pixels. This is done by using the height of the pixel in which each border pixel flows into
                for(register int i = 0; i < (*itBor).second.size(); i++ ) {
                    neighIt->SetLocation((*itBor).second[i]);
                    idIt->SetLocation( (*itBor).second[i] );
                    //finding maximum slope - SFDidMap->at(id).begin()
                    PixelType flowHeight = 9999;
                    float maxSlope = 0.0;
                    for (register int pos = 0; pos < 9; pos++) {
                        if   (  idIt->GetPixel(pos) < 1 )   {//the examined pixel is neither grouped nor border. This will suffice since the heights of the border pixels are the same
                            float tmpSlope = ( neighIt->GetCenterPixel() - neighIt->GetPixel( pos) ) / std :: sqrt ( std :: pow ( neighIt->GetOffset(pos)[0],2  ) + std :: pow (neighIt->GetOffset(pos)[1] , 2)  ) ;
                            if ( maxSlope < tmpSlope ) {
                                maxSlope = tmpSlope;
                                flowHeight = neighIt->GetPixel(pos);
                            }
                        }
                    }
                    if (maxSlope > 0.0 ) {
                        neighIt->SetCenterPixel( 0.9*neighIt->GetCenterPixel() + 0.1*flowHeight );
                    }
                }

            }//////
            else {//Trying to perform linear interpolation

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
                    //std :: cout << outlet.size() << "\t" << inflow.size() << std :: endl;
                    int outletToOutletCounter = 0;

                    for ( register int i = 0; i < outlet.size(); i++ ) {
                        double outHeight, outD;

                        double ang = std :: atan2 (  (*fs)[1] -  (*outlet[i])[1],  (*fs)[0] - (*outlet[i])[0] ) ;


                        outD = std :: sqrt (std ::  pow ( (*fs)[1] - (*outlet[i])[1] , 2 ) +  std :: pow ( (*fs)[0] - (*outlet[i])[0] , 2) );
                        int d = 10*outD + 1;

                        //determining if the OF line is inside the FAD
                        bool cont2fs = true;
                        //checking if the line from the outlet towards the pixel is completely inside the FAD


                        for ( register int m = 1;  ( (m <= d)  && (cont2fs == true) ); m++ ) {
                            ImageType::IndexType index;
                            index[0] = round ( (*outlet[i])[0] + m*0.1*cos(ang) );
                            index[1] = round ( (*outlet[i])[1] + m*0.1*sin(ang) );
                            idIt->SetLocation(index);

                            if  ( (index ==  (*outlet[i])  ) || ( idIt->GetCenterPixel() == (*itBor).first  )  ) {//the line is passing through the outlet or a pixel belonging to the group


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
                                        idIt->SetLocation(index);
                                        //checking if the pixel is inflow or outlet
                                        if ( ( idIt->GetCenterPixel() < 1 ) && ( neighIt->GetCenterPixel() > minHeight ) ) { //condition for boundary and inflow pixel
                                            double inHeight = neighIt->GetCenterPixel();
                                            double inD = std :: sqrt (std ::  pow ( (*fs)[1] - index[1] , 2 ) +  std :: pow ( (*fs)[0] - index[0] , 2) );
                                            PixelType tmpH = (outD*inHeight + inD*outHeight) / (outD + inD);
                                            if (finalHeights > tmpH)
                                                finalHeights = tmpH;
                                            cont2border = false;
                                        }
                                        else if ( (  idIt->GetCenterPixel() < 1 ) && ( neighIt->GetCenterPixel() == minHeight ) ) { //condition for boundary and outlet pixel - SPECIAL CASE 2
                                            outletToOutletCounter++;
                                            cont2border = false;
                                        }
                                    }
                                }
                            }
                            else { //in case when the line falls out of the FAD or the examined outlet
                                cont2fs = false;
                            }
                        }
                    }

                    if ( outletToOutletCounter == outlet.size() ) {
                        unprocessedFSCounter++;

                    }
                    else {

                        if (finalHeights != std :: pow(10, 6) ) {
                            neighIt->SetLocation(*fs);
                            neighIt->SetCenterPixel( finalHeights  + ((double) rand() / (RAND_MAX))*0.01 ); // set the result of linear interpolation

                        }
                    }

                }
                if ( unprocessedFSCounter == (*itMap).second.size()   ) {//all fs pixels remained unprocessed since special case 2 applied
                    //recompute heights for all outlet pixels
                    for(register int i = 0; i < outlet.size(); i++ ) {
                        neighIt->SetLocation( *outlet[i] );

                        //finding maximum slope - SFD
                        PixelType flowHeight = 9999;
                        float maxSlope = 0.0;

                        for (register int pos = 0; pos < 9; pos++) {
                            if ( pos != 4) {
                                idIt->SetLocation( neighIt->GetIndex( pos ) );
                                if (idIt->InBounds() ) {
                                    //std :: cout << neighIt->GetIndex(pos) <<"\n";
                                    if  ( idIt->GetCenterPixel() < 1.0 ) { //the examined pixel is neither grouped nor border. This suffice since we examine only outlet pixels, thus max slopes will occur only outside the border

                                        float tmpSlope = ( neighIt->GetCenterPixel() - neighIt->GetPixel(pos) )/std :: sqrt ( std :: pow ( neighIt->GetOffset(pos)[0],2  ) + std :: pow (neighIt->GetOffset(pos)[1] , 2)  ) ;
                                        //std :: cout <<(*itBor).first <<"\t" << neighIt->GetCenterPixel() <<"\t" << neighIt->GetPixel(pos) <<"\t" << tmpSlope <<"\n";
                                        if ( maxSlope < tmpSlope   ) {
                                            maxSlope = tmpSlope;
                                            flowHeight = neighIt->GetPixel(pos);
                                        }
                                    }
                                }
                            }
                        }

                        //std :: cout << "\n" <<(*itBor).first << "\t"  <<(*itBor).second[i] <<"\t" <<maxSlope <<"\n";
                        if (maxSlope > 0.0 ) {
                            neighIt->SetLocation( (*itBor).second[i] );
                            neighIt->SetCenterPixel( 0.9*neighIt->GetCenterPixel() + 0.1*flowHeight + ((double) rand() / (RAND_MAX))*0.01 );
                        }
                        else {
                            neighIt->SetLocation( (*itBor).second[i] );
                            neighIt->SetCenterPixel( flowHeight );
                        }
                    }
                }
                for (register int pp = 0; pp < (*itMap).second.size(); pp++ ) {
                    idIt->SetLocation ( (*itMap).second[pp] );
                    for(register int j = 0; j < 9; j++)
                        idIt->SetPixel( j, 0 );
                }



                outlet.clear();
                inflow.clear();
            }
        }

        if (idMap->size() > 0) {
            recompute = true;
            /*
            for (idIt->GoToBegin(); !idIt->IsAtEnd(); idIt->operator ++() ) {
                idIt->SetCenterPixel( 0 );
            }
            */
        }
        /*

        std :: stringstream ss2;
        ss2 <<"heights_";
        ss2 << reps;
        ss2  <<".img";
        WriterType :: Pointer   writer2 = WriterType :: New();
        writer2->SetFileName( ss2.str() );
        writer2->SetInput(tmpImage );
        writer2->Update();
        */

        idMap->clear();
        borderMap->clear();
        delete idMap;
        idMap = NULL;
        reps++;
        delete borderMap;
        borderMap = NULL;
        id = 1;
    } while(recompute);
}

void FlatSink :: setBorder ( mapIndexType *borderMap, mapIndexType *idMap )  {
    NeighborhoodIteratorType :: IndexType tmpIdx = idIt->GetIndex();

    for ( std :: vector<ConstIteratorType :: IndexType> :: iterator it = idMap->at(id).begin();    it != idMap->at(id).end(); it++  ) {
        idIt->SetLocation( *it );
        for (register int i = 0; i < 9; i++) {
            if ( ( i != 4 ) && (  idIt->GetPixel( i ) < 1 ) )
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

void FlatSink::writeImage( std::string fileName ) {
    WriterType :: Pointer   writer = WriterType :: New();
    writer->SetFileName(fileName);
    writer->SetInput(tmpImage);
    writer->Update();

}

