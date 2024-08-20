



#include "sierrachart.h"

#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

#define MAX_VECTOR_SIZE 2000


SCDLLName("SUA GOOD OF")

int uniqueNumberForGoodBuyOF = 23324 ;
int uniqueNumberForGoodSellOF = 13324 ;



// Structure to represent an good sell OF start bar
struct st_GoodSellOFStartBar {
	int index;
   
	st_GoodSellOFStartBar(int idx) : index(idx) {}
};

// Structure to represent an good sell Orderflow
struct st_GoodSellOF {
	int startBarIndex;
	int endBarIndex;
	int volume_of_start_bar ;
	int delta_of_start_bar ;
	int volume_of_end_bar ;
	int delta_of_end_bar ;
	SCDateTime time_of_start_bar ;
	SCDateTime time_of_end_bar ;
	int lineNumber;
	int isSameStartBar ;
  
	st_GoodSellOF(int idx1, int v1 , int d1  , SCDateTime t1 ,
				 int idx2, int v2 , int d2  , SCDateTime t2 ,
				 int ln , int i1) 
				 : startBarIndex(idx1), volume_of_start_bar(v1), delta_of_start_bar(d1) , time_of_start_bar(t1) ,
				   endBarIndex(idx2)  , volume_of_end_bar(v2)  , delta_of_end_bar(d2)   , time_of_end_bar(t2)   ,
				   lineNumber(ln) , isSameStartBar(i1) {}
	
};

// Structure to represent an good buy OF start bar
struct st_GoodBuyOFStartBar {
    int index;
   
	st_GoodBuyOFStartBar(int idx) : index(idx) {}
};

// Structure to represent an good buy Orderflow
struct st_GoodBuyOF {
    int startBarIndex;
	int endBarIndex;
	int volume_of_start_bar ;
	int delta_of_start_bar ;
	int volume_of_end_bar ;
	int delta_of_end_bar ;
    SCDateTime time_of_start_bar ;
	SCDateTime time_of_end_bar ;
    int lineNumber;
	int isSameStartBar ;
  
	st_GoodBuyOF(int idx1, int v1 , int d1  , SCDateTime t1 ,
	             int idx2, int v2 , int d2  , SCDateTime t2 ,
				 int ln  , int i1) 
				 : startBarIndex(idx1), volume_of_start_bar(v1), delta_of_start_bar(d1) , time_of_start_bar(t1) ,
				   endBarIndex(idx2)  , volume_of_end_bar(v2)  , delta_of_end_bar(d2)   , time_of_end_bar(t2)   ,
				   lineNumber(ln) , isSameStartBar(i1) {}
	
};

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);


int isThisStartBarOFGoodSellOrderflow(SCStudyInterfaceRef sc , int startIndex);


void findGoodSellOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , 
						  std::vector<st_GoodSellOF>& detectGoodSellOF);
						  

int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex);


void findGoodBuyOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , 
						  std::vector<st_GoodBuyOF>& detectGoodBuyOF);		


bool isBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar , int indexOfEndCheck)  ;

bool isBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar , int indexOfEndCheck) ;


SCSFExport scsf_GoodOF(SCStudyInterfaceRef sc)
{
	
	
	SCString msg;
	
	//SCInputRef i_pivotLength = sc.Input[count++];
	//SCInputRef i_tp_in_ticks = sc.Input[count++];
	//SCInputRef AlertSound = sc.Input[count++];
	// Array to store the first and last bar indexes for each day
	
	
	
    

	
	
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA GOOD OF";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 3;
		
		sc.UpdateAlways = 1;
		
		sc.Subgraph[0].Name = "good sell OF SIGNAL";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_BAR;
		sc.Subgraph[0].PrimaryColor = RGB(255,0,0);
        sc.Subgraph[0].LineWidth = 7;
	
		sc.Subgraph[1].Name = "good buy OF SIGNAL";
		sc.Subgraph[1].DrawStyle = DRAWSTYLE_BAR;
		sc.Subgraph[1].PrimaryColor = RGB(0,255,0);
        sc.Subgraph[1].LineWidth = 7;
		
		sc.Subgraph[2].Name = "good buy start index";
		sc.Subgraph[2].DrawStyle = DRAWSTYLE_HIDDEN;
		
		sc.Subgraph[3].Name = "good buy end index";
		sc.Subgraph[3].DrawStyle = DRAWSTYLE_HIDDEN;
		
		sc.Subgraph[4].Name = "good sell start index";
		sc.Subgraph[4].DrawStyle = DRAWSTYLE_HIDDEN;
		
		sc.Subgraph[5].Name = "good sell end index";
		sc.Subgraph[5].DrawStyle = DRAWSTYLE_HIDDEN;
		
	
		
		return;
	}
	
	
	// Section 2 - Do data processing here
	
	
	
	
	
	
	
	
	int MaxBarLookback = MAX_VECTOR_SIZE;
	int MIN_START_INDEX = 2;   // need 3 so it is 0 1 2
	
	
	
	
	auto detectedGoodSellStartBars = static_cast<std::vector<st_GoodSellOFStartBar>*>(sc.GetPersistentPointer(3));
	auto detectGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(4));
	
	auto detectedGoodBuyStartBars = static_cast<std::vector<st_GoodBuyOFStartBar>*>(sc.GetPersistentPointer(1));
	auto detectGoodBuyOF = static_cast<std::vector<st_GoodBuyOF>*>(sc.GetPersistentPointer(2));
	
	
		
	if (!detectedGoodSellStartBars) {
        detectedGoodSellStartBars = new std::vector<st_GoodSellOFStartBar>();
        sc.SetPersistentPointer(3, detectedGoodSellStartBars);
    }	
	
	if (!detectGoodSellOF) {
        detectGoodSellOF = new std::vector<st_GoodSellOF>();
        sc.SetPersistentPointer(4, detectGoodSellOF);
    }
	
	if (!detectedGoodBuyStartBars) {
        detectedGoodBuyStartBars = new std::vector<st_GoodBuyOFStartBar>();
        sc.SetPersistentPointer(1, detectedGoodBuyStartBars);
    }
	
	if (!detectGoodBuyOF) {
        detectGoodBuyOF = new std::vector<st_GoodBuyOF>();
        sc.SetPersistentPointer(2, detectGoodBuyOF);
    }

		


	
	if (MaxBarLookback == 0 )
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;
	
	
	
	
	//A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		
		
		
		for (int i = 0; i < detectGoodSellOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodSellOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodSellOF->clear();
		
		
		for (int i = 0; i < detectGoodBuyOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodBuyOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodBuyOF->clear();
				
				
			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	// ################################         MUST clear vector for                ###########################################
	// ################################      1. help speed up 3 times                ###########################################
	// ################################      2. protect it add same data	         ###########################################
	
	
		
	if (detectedGoodSellStartBars != NULL)
		detectedGoodSellStartBars->clear();
	
	if (detectGoodSellOF != NULL)
		detectGoodSellOF->clear();
	
	
	if (detectedGoodBuyStartBars != NULL)
		detectedGoodBuyStartBars->clear();
	
	
	if (detectGoodBuyOF != NULL)
		detectGoodBuyOF->clear();
	
	
	
	
			
	//  *************      1. FIND GOOD SELL AND GOOD BUY Start Bar         ****************************** 
	bool lastBarIsGoodSellStartBar = false;
	for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{	
				
		bool isItStartBarOFGoodSellOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATTOP);
		bool isItStartBarOFGoodBuyOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM);
		
		// if found good Sell start bar , then add it to vector
		if(isItStartBarOFGoodSellOrderflow)
		{
						
			// use emplace_back
			detectedGoodSellStartBars->emplace_back(i);			
		
				
			// Ensure the vector does not exceed the maximum size
			if (detectedGoodSellStartBars->size() > MAX_VECTOR_SIZE) {
				detectedGoodSellStartBars->erase(detectedGoodSellStartBars->begin());
			}				
			
		}				
		
		
		
		// if found good buy start bar , then add it to vector
		if(isItStartBarOFGoodBuyOrderflow)
		{
						
			// use emplace_back
			detectedGoodBuyStartBars->emplace_back(i);			
			
			// Ensure the vector does not exceed the maximum size
			if (detectedGoodBuyStartBars->size() > MAX_VECTOR_SIZE) {
				detectedGoodBuyStartBars->erase(detectedGoodBuyStartBars->begin());
			}
		
		}
		
		
		
	}
	
	/*int currentIndex = sc.UpdateStartIndex ;
	
	if(currentIndex >= sc.DataStartIndex && sc.Subgraph[0][currentIndex-1] == 1)
		sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , "good 8 sell start bar" );*/
	
	
	
	 // ***************      2. find end bar of good sell / good buy OF       ********************************
	findGoodSellOrderFlow(sc , *detectedGoodSellStartBars , *detectGoodSellOF);
	
	findGoodBuyOrderFlow(sc , *detectedGoodBuyStartBars , *detectGoodBuyOF);
	
	
	
	
	// ****************     3. assign is same start bar or not        *****************************
	SCString msg2;
	for (int i = 0 ; i < detectGoodSellOF->size() - 1 ; i++)   
	{
		msg2.Format("i : %d  \n" , i  );
		//sc.AddMessageToLog(msg2,0);
		if(i>=1)
		{
			for( int j = i-1 ; j >= 0 ; j--)
			{
				if(detectGoodSellOF->at(i).startBarIndex == detectGoodSellOF->at(j).startBarIndex)
				{
					detectGoodSellOF->at(i).isSameStartBar = 1 ;
					
					msg2.Format("found same start bar : %d | %d \n" , detectGoodSellOF->at(i).startBarIndex , detectGoodSellOF->at(i).endBarIndex );
					//sc.AddMessageToLog(msg2,0);
					//check no candle go above high of start bar
					break;
				}
			}
			
		}
	}
	
	for (int i = 0 ; i < detectGoodBuyOF->size() - 1 ; i++)   
	{
		msg2.Format("i : %d  \n" , i  );
		//sc.AddMessageToLog(msg2,0);
		if(i>=1)
		{
			for( int j = i-1 ; j >= 0 ; j--)
			{
				if(detectGoodBuyOF->at(i).startBarIndex == detectGoodBuyOF->at(j).startBarIndex)
				{
					detectGoodBuyOF->at(i).isSameStartBar = 1 ;
					
					msg2.Format("found same start bar : %d | %d \n" , detectGoodBuyOF->at(i).startBarIndex , detectGoodBuyOF->at(i).endBarIndex );
					//sc.AddMessageToLog(msg2,0);
					//check no candle go above high of start bar
					break;
				}
			}
			
		}
	}
		
/*		
		
	// **************     4. put signal in subgraph       *******************************
	
	for (int i = detectGoodSellOF->size() - 1 ; i >= 0; i--)   
	//for (int i = 0 ; i < detectGoodSellOF->size() - 1 ; i++)  
	{
		if(detectGoodSellOF->at(i).isSameStartBar == 1) continue ;
		
		sc.Subgraph[0][detectGoodSellOF->at(i).endBarIndex] = 1;
		sc.Subgraph[4][detectGoodSellOF->at(i).endBarIndex] = detectGoodSellOF->at(i).startBarIndex ;
		sc.Subgraph[5][detectGoodSellOF->at(i).endBarIndex] = detectGoodSellOF->at(i).endBarIndex ;
		
	//	if(detectGoodSellOF->at(i).isSameStartBar == 0)
	//	{
	//		sc.Subgraph[0][detectGoodSellOF->at(i).endBarIndex] = 1;
	//	}
	//	else
	//	{
	//		//sc.Subgraph[0][detectGoodSellOF->at(i).endBarIndex] = 0;
	//	}
		
	}
	
	for (int i = detectGoodBuyOF->size() - 1 ; i >= 0; i--)   	
	{
		if(detectGoodBuyOF->at(i).isSameStartBar == 1) continue ;
		
		sc.Subgraph[1][detectGoodBuyOF->at(i).endBarIndex] = 1;		
		sc.Subgraph[2][detectGoodBuyOF->at(i).endBarIndex] = detectGoodBuyOF->at(i).startBarIndex;			
		sc.Subgraph[3][detectGoodBuyOF->at(i).endBarIndex] = detectGoodBuyOF->at(i).endBarIndex;	
		
	}
	
	*/
	
	// **************     4. put signal in subgraph       *******************************
	
	for ( int i = sc.UpdateStartIndex ; i < sc.ArraySize-1; ++i) 
	{
		for (int j = detectGoodSellOF->size() - 1 ; j >= 0; j--)   
		//for (int i = 0 ; i < detectGoodSellOF->size() - 1 ; i++)  
		{
			if(detectGoodSellOF->at(j).isSameStartBar == 1) continue ;
			
			sc.Subgraph[0][detectGoodSellOF->at(j).endBarIndex] = 1;
			sc.Subgraph[4][detectGoodSellOF->at(j).endBarIndex] = detectGoodSellOF->at(j).startBarIndex ;
			sc.Subgraph[5][detectGoodSellOF->at(j).endBarIndex] = detectGoodSellOF->at(j).endBarIndex ;
		
		}
	
		for (int j = detectGoodBuyOF->size() - 1 ; j >= 0; j--)   	
		{
			if(detectGoodBuyOF->at(j).isSameStartBar == 1) continue ;
			
			sc.Subgraph[1][detectGoodBuyOF->at(j).endBarIndex] = 1;		
			sc.Subgraph[2][detectGoodBuyOF->at(j).endBarIndex] = detectGoodBuyOF->at(i).startBarIndex;			
			sc.Subgraph[3][detectGoodBuyOF->at(j).endBarIndex] = detectGoodBuyOF->at(i).endBarIndex;	
			
		}
		
	
		
	/*  for (int j = detectGoodSellOF->size() - 1 ; j >= 0; j--)   
		//for (int i = 0 ; i < detectGoodSellOF->size() - 1 ; i++)  
		{
			//if(detectGoodSellOF->at(j).isSameStartBar == 1) continue ;
			
			if(i == detectGoodSellOF->at(j).endBarIndex && detectGoodSellOF->at(j).isSameStartBar == 0)
			{
				sc.Subgraph[0][i] = 1;
				sc.Subgraph[4][i] = detectGoodSellOF->at(j).startBarIndex ;
				sc.Subgraph[5][i] = detectGoodSellOF->at(j).endBarIndex ;
				break ;
			}			
		
		}
		
		for (int j = detectGoodBuyOF->size() - 1 ; j >= 0; j--)   	
		{
			//if(detectGoodBuyOF->at(j).isSameStartBar == 1) continue ;
			
			if(i == detectGoodBuyOF->at(j).endBarIndex && detectGoodBuyOF->at(j).isSameStartBar == 0)
			{
				sc.Subgraph[1][i] = 1;		
				sc.Subgraph[2][i] = detectGoodBuyOF->at(i).startBarIndex;			
				sc.Subgraph[3][i] = detectGoodBuyOF->at(i).endBarIndex;	
				break ;
			}
		}
		*/
		
	}
	
	
	
	
	// *******************     5.  draw good buy sell OF     ***************************
	for (int i = detectGoodSellOF->size() - 1 ; i >= 0; i--)   
	{
		
		if(detectGoodSellOF->at(i).isSameStartBar == 1) continue ;		
		
		int st_index = detectGoodSellOF->at(i).startBarIndex;
		int ed_index = detectGoodSellOF->at(i).endBarIndex;
		float high = sc.High[st_index] ;
		float low = sc.Low[ed_index] ;
		
		// draw good sell OF
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = st_index;	
		rectangle.EndIndex = ed_index; 
		rectangle.BeginValue = high ;
		rectangle.EndValue = low ;
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectGoodSellOF->at(i).lineNumber ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);			
	
	}	
	
	for (int i = detectGoodBuyOF->size() - 1 ; i >= 0; i--)   
	{
		
		if(detectGoodBuyOF->at(i).isSameStartBar == 1) continue ;		
		
		int st_index = detectGoodBuyOF->at(i).startBarIndex;
		int ed_index = detectGoodBuyOF->at(i).endBarIndex;
		float high = sc.High[ed_index] ;
		float low = sc.Low[st_index] ;
		
		// draw good sell OF
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = st_index;	
		rectangle.EndIndex = ed_index; 
		rectangle.BeginValue = high ;
		rectangle.EndValue = low ;
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 255, 0);  // yellow color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectGoodBuyOF->at(i).lineNumber ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
			
	
	}	
	
	
	
	
	// *******************     6. count good OF each day     ***************************
	SCDateTime currentDate = sc.BaseDateTimeIn[sc.ArraySize - 1].GetDate();
	
	for (int i = 0; i < 10; ++i)
    {
        SCDateTime dateToPrint = currentDate - i;

        // Format the date into a string (YYYY-MM-DD)
        SCString dateString2;
		
		int firstIndexToday = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, dateToPrint);
		SCDateTime t1 = sc.BaseDateTimeIn[firstIndexToday];	
		if(t1.GetDay() < dateToPrint.GetDay()) firstIndexToday++ ;		
		
		SCDateTime tomorrowDate = dateToPrint+1;
		int firstIndexOfTomorrow = sc.GetNearestMatchForSCDateTime(sc.ChartNumber, tomorrowDate);		
		
		int dayOfWeek = dateToPrint.GetDayOfWeek();
		
		int countGoodSell = 0;
		int countGoodBuy = 0;
		int countLossSell = 0;
		int countLossBuy = 0;
		 for(int j = firstIndexToday ; j < firstIndexOfTomorrow ; j++)
		{
			if (sc.Subgraph[0][j] == 1 )
			{
				if (sc.Subgraph[4][j] != 0 ) countGoodSell++ ;
				
				int st_index = static_cast<int>(sc.Subgraph[4][j]);
				int ed_index = j;  
				
				if(isBreakHigh(sc,st_index ,ed_index , firstIndexOfTomorrow ) == true ) countLossSell++;
				
			}				
			
			if (sc.Subgraph[1][j] == 1 )
			{
				if (sc.Subgraph[2][j] != 0 ) countGoodBuy++ ;
				
				int st_index = static_cast<int>(sc.Subgraph[2][j]);
				int ed_index = j;
				
				//dateString2.Format("st idx = %d | ex idx = %d | tomorrow idx = %d " , st_index ,ed_index , firstIndexOfTomorrow );
				//sc.AddMessageToLog(dateString2, 0);

				
				if(isBreakLow(sc,st_index ,ed_index , firstIndexOfTomorrow ) == true ) countLossBuy++;
			}
				
			
			
			
			
			
			//if(isBreakLow(sc,st_index ,ed_index , firstIndexOfTomorrow ) == true ) countLossBuy++;
			
		}		
		
        dateString2.Format(
            "%04d-%02d-%02d %02d:%02d:%02d | first idx = %d | day %d | count good buy = %d | count good sell = %d"  , 
            dateToPrint.GetYear(), dateToPrint.GetMonth(), dateToPrint.GetDay(),
            dateToPrint.GetHour(), dateToPrint.GetMinute(), dateToPrint.GetSecond()  ,     
			firstIndexToday , dayOfWeek , countGoodBuy ,countGoodSell 
        );

        // Print the date to the log
        //sc.AddMessageToLog(dateString2, 0);	
		
		//dateString2.Format("lost buy = %d | lost sell = %d" , countLossBuy , countLossSell);
		//sc.AddMessageToLog(dateString2, 0);
		
		
	}

}



bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition)
{
	const s_VolumeAtPriceV2* p_vap = NULL;	
	int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(idx) ;	
	int vol;	
	int levelOFHighestVol ;	
	int highestVol = 0;	
		
	if(condition == CHECK_5LEVEL)
	{
		if(numPriceLevel == 5)
			return true;
		else
			return false;
	}
	else if(condition == CHECK_5LEVEL_AND_POCATBOTTOM)
	{
		if(numPriceLevel == 5)
		{}
	    else return false;
		
		// find POC level
		for(int pocLevel = 0 ; pocLevel < numPriceLevel ; pocLevel++)
		{
					
			sc.VolumeAtPriceForBars->GetVAPElementAtIndex(idx , pocLevel ,&p_vap );
			
			if(p_vap->Volume > highestVol)
			{
				highestVol = p_vap->Volume ;
				levelOFHighestVol = pocLevel;
				
			}		
			else if(p_vap->Volume == highestVol)
			{
				// TODO
				
			}			
			
			vol = p_vap->Volume ;		
			
		}
		
		if(levelOFHighestVol <= 1) 
		{
			//SCString msg;
			//msg.Format("------------- index %d is valid start bar | poc level = %d | highest vol = %d | " , idx , levelOFHighestVol , highestVol );
			//sc.AddMessageToLog(msg,0);
			return true;
		}
		else 
		{
			return false;
		}
	}
	else if(condition == CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE)
	{
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
		// check is up candle ?
		if ( (sc.Close[idx] - sc.Low[idx]) > 0);
			//return true;
	    else
	   	   return false;
	   		
		// find POC level
		for(int pocLevel = 0 ; pocLevel < numPriceLevel ; pocLevel++)
		{
					
			sc.VolumeAtPriceForBars->GetVAPElementAtIndex(idx , pocLevel ,&p_vap );
			
			if(p_vap->Volume > highestVol)
			{
				highestVol = p_vap->Volume ;
				levelOFHighestVol = pocLevel;
				
			}
			else if(p_vap->Volume == highestVol)
			{
				// TODO
				
			}
			
			vol = p_vap->Volume ;		
			
		}
		
		if(levelOFHighestVol >= 3) return true;
		else return false;
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATTOP)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
		// find POC level
		for(int pocLevel = 0 ; pocLevel < numPriceLevel ; pocLevel++)
		{
					
			sc.VolumeAtPriceForBars->GetVAPElementAtIndex(idx , pocLevel ,&p_vap );
			
			if(p_vap->Volume > highestVol)
			{
				highestVol = p_vap->Volume ;
				levelOFHighestVol = pocLevel;
				
			}		
			else if(p_vap->Volume == highestVol)
			{
				// TODO
				
			}			
			
			vol = p_vap->Volume ;		
			
		}
		
		if(levelOFHighestVol >= 3) 
		{
			SCString msg;
			msg.Format("------------- index %d is valid start bar | poc level = %d | highest vol = %d | " , idx , levelOFHighestVol , highestVol );
			//sc.AddMessageToLog(msg,0);
			return true;
		}
		else 
		{
			return false;
		}
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
		// check is down candle ?
		if ( (sc.Close[idx] - sc.Low[idx]) > 0)
			return false;
	    
	   	 
		
		// find POC level
		for(int pocLevel = 0 ; pocLevel < numPriceLevel ; pocLevel++)
		{
					
			sc.VolumeAtPriceForBars->GetVAPElementAtIndex(idx , pocLevel ,&p_vap );
			
			if(p_vap->Volume > highestVol)
			{
				highestVol = p_vap->Volume ;
				levelOFHighestVol = pocLevel;
				
			}		
			else if(p_vap->Volume == highestVol)
			{
				// TODO
				
			}			
			
			vol = p_vap->Volume ;		
			
		}
		
		
	   
		if(levelOFHighestVol <= 1) 
		{
			//SCString msg;
			//msg.Format("------------- index %d is valid start bar | poc level = %d | highest vol = %d | " , idx , levelOFHighestVol , highestVol );
			//sc.AddMessageToLog(msg,0);
			return true;
		}
		else 
		{
			return false;
		}
		
		
		
	}
	
	return false;
	
}


void findGoodSellOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , std::vector<st_GoodSellOF>& detectGoodSellOF) 
{
	int lowOfBar  ;
	int highOfBar ;
	int lowOfStartBar ;
	int highOfStartBar ;
			
    // loop from first index of start bar of good Sell OF to last 
	for (const auto& bar : detectedGoodSellStartBars) {
        int startBarIndex = bar.index;
		SCString msg;
		msg.Format("low of index %d is valid start bar | pRICE = %d  " , bar.index , sc.PriceValueToTicks(sc.Low[bar.index])  );
		//sc.AddMessageToLog(msg,0);
			
		// loop from start bar to current bar
        for (int j = startBarIndex + 1; j < sc.ArraySize - 1; ++j) 
		{
			// check if start bar and end bar far apart 1 Tick ?
			lowOfBar = sc.PriceValueToTicks(sc.Low[j]) ;
			highOfBar = sc.PriceValueToTicks(sc.High[j]) ;
			lowOfStartBar = sc.PriceValueToTicks(sc.Low[startBarIndex]) ;
			highOfStartBar = sc.PriceValueToTicks(sc.High[startBarIndex]) ;			
			
			
            if((highOfBar <= lowOfStartBar + 1) && (highOfBar >= lowOfStartBar - 1))
			{
			
				// check end bar is 5 level , poc on bottom , down candle
                if (checkValidBar(sc, j, CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE)) 
				{
					SCString msg;
					msg.Format("found valid poc at bottom and down candle start bar : %d | end bar : %d \n" , startBarIndex , j );
					//sc.AddMessageToLog(msg,0);
					// check no candle go above high of start bar
					bool noCandlebreakhigh = true;
					for(int k = startBarIndex ; k <= j ; ++k )
					{
						if(sc.High[k] > sc.High[startBarIndex])
						{
							noCandlebreakhigh = false ;
							break;
						}
					}					
					// draw if no candle break high of start bar
					if(noCandlebreakhigh)
					{
						int idx1 = startBarIndex;
						int v1 = sc.Volume[startBarIndex];
						int d1 = 0;
						SCDateTime t1 = sc.BaseDateTimeIn[startBarIndex];
						
						int idx2 = j ;
						int v2 = sc.Volume[j];
						int d2 = 0;
						SCDateTime t2 = sc.BaseDateTimeIn[j];	

                        int ln = uniqueNumberForGoodSellOF + startBarIndex + j ;	
						int i1 = 0;
						
											
						//auto& data = detectedGoodBuyStartBars;
						detectGoodSellOF.emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln , i1);
												
						/*
						//bar.lineNumber = uniqueNumber + startBarIndex + j ;	
						
						// **************************** Draw GOOD BUY OF HERE *********************************************
						s_UseTool rectangle;
						rectangle.Clear();
						rectangle.ChartNumber = sc.ChartNumber;
						rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
						rectangle.AddAsUserDrawnDrawing = 1;
						rectangle.BeginIndex = startBarIndex;
						rectangle.EndIndex = j;
						rectangle.BeginValue = sc.High[j];
						rectangle.EndValue = sc.Low[startBarIndex];
						rectangle.Color = RGB(255, 0, 0);
						rectangle.SecondaryColor = RGB(255, 255, 0);  // Yellow color
						rectangle.LineWidth = 1;
						rectangle.TransparencyLevel = 75;
						rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
						rectangle.LineNumber = ln ;
						sc.UseTool(rectangle);
						
						SCString msg;
						msg.Format("start bar : %d | end bar : %d \n" , startBarIndex , j );
						sc.AddMessageToLog(msg,0);
						
						*/
					}
                 
                }
            }
        }
    }
}

void findGoodBuyOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , std::vector<st_GoodBuyOF>& detectGoodBuyOF) 
{
    // loop from first index of start bar of good buy OF to last 
	for (const auto& bar : detectedGoodBuyStartBars) {
        int startBarIndex = bar.index;
		// loop from start bar to current bar
        for (int j = startBarIndex + 1; j < sc.ArraySize - 1; ++j) 
		{
			// check if start bar and end bar far apart 1 Tick ?
            if ((sc.Low[j] >= sc.High[startBarIndex] - 0.00011) && (sc.Low[j] <= sc.High[startBarIndex] + 0.00011))
			{
				// check end bar is 5 level , poc on top , up candle
                if (checkValidBar(sc, j, CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE)) 
				{
					// check no candle go below low of start bar
					bool noCandlebreakLow = true;
					for(int k = startBarIndex ; k <= j ; ++k )
					{
						if(sc.Low[k] < sc.Low[startBarIndex])
						{
							noCandlebreakLow = false ;
							break;
						}
					}					
					// draw if no candle break low of start bar
					if(noCandlebreakLow)
					{
						int idx1 = startBarIndex;
						int v1 = sc.Volume[startBarIndex];
						int d1 = 0;
						SCDateTime t1 = sc.BaseDateTimeIn[startBarIndex];
						
						int idx2 = j ;
						int v2 = sc.Volume[j];
						int d2 = 0;
						SCDateTime t2 = sc.BaseDateTimeIn[j];	

                        int ln = uniqueNumberForGoodBuyOF + startBarIndex + j ;						
						int i1 = 0;
											
						//auto& data = detectedGoodBuyStartBars;
						detectGoodBuyOF.emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln , i1 );
												
						/*
						//bar.lineNumber = uniqueNumber + startBarIndex + j ;	
						
						// **************************** Draw GOOD BUY OF HERE *********************************************
						s_UseTool rectangle;
						rectangle.Clear();
						rectangle.ChartNumber = sc.ChartNumber;
						rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
						rectangle.AddAsUserDrawnDrawing = 1;
						rectangle.BeginIndex = startBarIndex;
						rectangle.EndIndex = j;
						rectangle.BeginValue = sc.High[j];
						rectangle.EndValue = sc.Low[startBarIndex];
						rectangle.Color = RGB(255, 0, 0);
						rectangle.SecondaryColor = RGB(255, 255, 0);  // Yellow color
						rectangle.LineWidth = 1;
						rectangle.TransparencyLevel = 75;
						rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
						rectangle.LineNumber = ln ;
						sc.UseTool(rectangle);
						
						SCString msg;
						msg.Format("start bar : %d | end bar : %d \n" , startBarIndex , j );
						sc.AddMessageToLog(msg,0);
						
						*/
					}
                 
                }
            }
        }
    }
}


bool isBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar , int indexOfEndCheck) 
{
	int sl = 	sc.PriceValueToTicks(sc.Low[indexOfStartBar]) - 1 ;// SL at 
	
	int indexThatBreakLow = -1;
	
	// find which index break low
	for(int i = indexOfEndtBar+1 ; i < indexOfEndCheck ; i++)
	{
		if( sc.PriceValueToTicks(sc.Low[i]) <= sl )
		{
			indexThatBreakLow = i;
			//break;
			return true;
		}
	}
	
	return false;
	
	/*if(indexThatBreakLow != -1)
	{
		return true;
	}
	else
	{
		return false;
	}
	*/
	
	
	
}


bool isBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar , int indexOfEndCheck) 
{
	int sl = 	sc.PriceValueToTicks(sc.High[indexOfStartBar]) + 1 ;// SL at 
	
	int indexThatBreakHigh = -1;
	
	// find which index break high
	for(int i = indexOfEndtBar+1 ; i < indexOfEndCheck ; i++)
	{
		if( sc.PriceValueToTicks(sc.High[i]) >= sl )
		{
			indexThatBreakHigh = i;
			//break;
			return true;
		}
	}
	
	return false;
	
	
	
	
	
}













