



#include "sierrachart.h"

#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

#define MAX_VECTOR_SIZE 600


SCDLLName("SUA ALERT DT GOOD OF")

int uniqueNumberForPivot = 8886 ;
int uniqueNumberForGoodSellOF = 2344 ;

// Structure to represent an PivotHigh
struct st_PivotHigh {
	int index;
	int lineNumber;
   
	st_PivotHigh(int idx , int ln) : index(idx) ,lineNumber(ln) {}
};

// Structure to represent an PivotLow
struct st_PivotLow {
	int index;
	int lineNumber;
   
	st_PivotLow(int idx , int ln) : index(idx) ,lineNumber(ln) {}
};

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
  
	st_GoodSellOF(int idx1, int v1 , int d1  , SCDateTime t1 ,
				 int idx2, int v2 , int d2  , SCDateTime t2 ,
				 int ln ) 
				 : startBarIndex(idx1), volume_of_start_bar(v1), delta_of_start_bar(d1) , time_of_start_bar(t1) ,
				   endBarIndex(idx2)  , volume_of_end_bar(v2)  , delta_of_end_bar(d2)   , time_of_end_bar(t2)   ,
				   lineNumber(ln) {}
	
};

// Structure to represent an good sell OF start bar
struct st_DoubleTopWithGoodOF {
	int lineNumberOf3tzone;
	int lineNumberOfGoodOf;
	int indexOfPivotHigh;
	int indexOfGoodOfStartBar;
	int indexOfGoodOfEndBar;
	float highOfPivotHigh;
	float highOfGoodOF;
	float lowOfGoodOF;
   
	st_DoubleTopWithGoodOF(int i1, int i2 , int i3 , int i4 , int i5 ,
	                       float f1 , float f2 , float f3) 
						   : lineNumberOf3tzone(i1), lineNumberOfGoodOf(i2), indexOfPivotHigh(i3) ,indexOfGoodOfStartBar(i4) , indexOfGoodOfEndBar(i5) ,
 						     highOfPivotHigh(f1) , highOfGoodOF(f2) , lowOfGoodOF(f3) {}
};


bool iSPivotHigh(SCStudyInterfaceRef sc , int index, int pivotLength) ;

int isThisStartBarOFGoodSellOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodSellOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , 
						  std::vector<st_GoodSellOF>& detectGoodSellOF);
						  
float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );



SCSFExport scsf_AlertDTWithGoodOF(SCStudyInterfaceRef sc)
{
	
	int count = 0;
	SCString msg;
	
	SCInputRef i_pivotLength = sc.Input[count++];
	SCInputRef i_tp_in_ticks = sc.Input[count++];
	SCInputRef AlertSound = sc.Input[count++];
	
	
	
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA ALERT DT WITH GOOD OF";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 0;
		
		sc.Subgraph[0].Name = "SELL SIGNAL";
		
		i_pivotLength.Name = "number of bar each side | ex. 4 is left 4 bars , right 4 bars";
		i_pivotLength.SetInt(4);
		
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(40);
		
	
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(0);  // Default alert sound number		
		
		sc.AlertOnlyOncePerBar = true;
		sc.ResetAlertOnNewBar = true; 
		
		return;
	}
	
	
	// Section 2 - Do data processing here
	
	int MaxBarLookback = MAX_VECTOR_SIZE;
	int MIN_START_INDEX = 2;   // need 3 so it is 0 1 2
	
	int pivotLength = i_pivotLength.GetInt();
	int tp_in_ticks = i_tp_in_ticks.GetInt();
	
	auto detectPivotHighs = static_cast<std::vector<st_PivotHigh>*>(sc.GetPersistentPointer(1)); 
	auto detectedGoodSellStartBars = static_cast<std::vector<st_GoodSellOFStartBar>*>(sc.GetPersistentPointer(3));
	auto detectGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(4));
	auto detectDTWithGoodOF = static_cast<std::vector<st_DoubleTopWithGoodOF>*>(sc.GetPersistentPointer(5));
	
	
	
	if (!detectPivotHighs) {
        detectPivotHighs = new std::vector<st_PivotHigh>();
        sc.SetPersistentPointer(1, detectPivotHighs);
    }
	
		
	if (!detectedGoodSellStartBars) {
        detectedGoodSellStartBars = new std::vector<st_GoodSellOFStartBar>();
        sc.SetPersistentPointer(3, detectedGoodSellStartBars);
    }	
	
	if (!detectGoodSellOF) {
        detectGoodSellOF = new std::vector<st_GoodSellOF>();
        sc.SetPersistentPointer(4, detectGoodSellOF);
    }
	
	if (!detectDTWithGoodOF) {
        detectDTWithGoodOF = new std::vector<st_DoubleTopWithGoodOF>();
        sc.SetPersistentPointer(5, detectDTWithGoodOF);
    }
	
		
	
	
	if(pivotLength > MIN_START_INDEX)
	{
		MIN_START_INDEX = pivotLength*2 ;
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
		
		for (int i = 0; i < detectPivotHighs->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectPivotHighs->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectPivotHighs->clear();
		
		
		for (int i = 0; i < detectGoodSellOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodSellOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodSellOF->clear();
		
		for (int i = 0; i < detectDTWithGoodOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectDTWithGoodOF->at(i).lineNumberOf3tzone);
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectDTWithGoodOF->at(i).lineNumberOfGoodOf);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectDTWithGoodOF->clear();
				
				
			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	// ################################         MUST clear vector for                ###########################################
	// ################################      1. help speed up 3 times                ###########################################
	// ################################      2. protect it add same data	         ###########################################
	
	if (detectPivotHighs != NULL)
		detectPivotHighs->clear();
		
	if (detectedGoodSellStartBars != NULL)
		detectedGoodSellStartBars->clear();
	
	if (detectGoodSellOF != NULL)
		detectGoodSellOF->clear();
	
	if (detectDTWithGoodOF != NULL)
		detectDTWithGoodOF->clear();
	
	
	
	 // 1.Loop through bars to DT  pattern
    for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{		
		// Check if this is a new bar
		//if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED) 
          //  continue;
        
		
		bool checkPivotHigh = iSPivotHigh(sc , i ,pivotLength );
	
		int ln = uniqueNumberForPivot+i ;
		
		// if found pivot high , then add it to vector
		if(checkPivotHigh)
		{						
			// use emplace_back
			detectPivotHighs->emplace_back(i ,ln );			
			
			// Ensure the vector does not exceed the maximum size
			if (detectPivotHighs->size() > MAX_VECTOR_SIZE) {
				detectPivotHighs->erase(detectPivotHighs->begin());
			}
			
			
		}					
		
			
	}	
	
	
	//  2. FIND GOOD SELL Start Bar 
	bool lastBarIsGoodSellStartBar = false;
	for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{	
				
		bool isItStartBarOFGoodSellOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATTOP);
		
		
		// if found good Sell start bar , then add it to vector
		if(isItStartBarOFGoodSellOrderflow)
		{
						
			// use emplace_back
			detectedGoodSellStartBars->emplace_back(i);			
		
				
			// Ensure the vector does not exceed the maximum size
			if (detectedGoodSellStartBars->size() > MAX_VECTOR_SIZE) {
				detectedGoodSellStartBars->erase(detectedGoodSellStartBars->begin());
			}		
			
			
			sc.Subgraph[0][i] = 1;
			
			if(i == sc.ArraySize-1-1 )
			{
				// Trigger the alert
				
				lastBarIsGoodSellStartBar = true;
				//break;
			}

		}		
		else
		{
			sc.Subgraph[0][i] = 0;
		}
	}
	
	int currentIndex = sc.UpdateStartIndex ;
	
	if(currentIndex >= sc.DataStartIndex && sc.Subgraph[0][currentIndex-1] == 1)
		sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , "good 8 sell start bar" );
	
	
	
	 // 2. find end bar of good sell OF
	findGoodSellOrderFlow(sc , *detectedGoodSellStartBars , *detectGoodSellOF);
	
	
	// 3. loop from start of good sell OF
	//for (const auto& goodSellOF : *detectGoodSellOF)
	for (int i = 0 ; i < detectGoodSellOF->size() ; i++)
	{
		int indexOfStartBar = detectGoodSellOF->at(i).startBarIndex;
		float highOfStartBar = sc.High[indexOfStartBar] ;
		// find at left side of Good OF have pivot high or not
		for(int j = (int)detectPivotHighs->size() - 1 ; j >= 0 ; j--)
		{
			SCString msg;
			//msg.Format("j : %d \n" ,j );
			//sc.AddMessageToLog(msg,0);
			int indexOfPivotHigh = detectPivotHighs->at(j).index;
			// if find index of bar of pivot high at leftside
			if( indexOfPivotHigh < indexOfStartBar)
			{
				// check if high not +- more3T
				float highOfPivot = sc.High[detectPivotHighs->at(j).index];
				int p = sc.PriceValueToTicks(highOfPivot) -  sc.PriceValueToTicks(highOfStartBar) ;
				if(abs(p) <= 3 )
				{
					bool noCandleBreakHigh = true;
					float maxValue = max(highOfPivot, highOfStartBar) ;
					// loop from start bar to pivot high ,then check if no candle break high of both
					for(int k = indexOfStartBar-1 ; k >= indexOfPivotHigh ; k--)
					{
						
						if(sc.High[k] > maxValue)
						{
							noCandleBreakHigh = false ;
							break;
						}
					}
					if(noCandleBreakHigh)
					{
												
						int ln1 = detectPivotHighs->at(j).lineNumber;
						int ln2 = detectGoodSellOF->at(i).lineNumber;
						int idxPivotHigh = indexOfPivotHigh ;
						int idxStartBar = indexOfStartBar ;
						int idxOFEndBar = detectGoodSellOF->at(i).endBarIndex;
						float hghOfPivot = highOfPivot ;
						float hghOfGoodOF = sc.High[idxStartBar] ;
						float lowOfGoodOF = sc.Low[idxOFEndBar] ;
							 
						detectDTWithGoodOF->emplace_back(ln1 , ln2 ,idxPivotHigh , idxStartBar , idxOFEndBar , hghOfPivot , hghOfGoodOF , lowOfGoodOF);	
						
					
						
						
					}
						
				}
			}
			
		}
	}
	
	
	// 4. Alert , draw 3T zone , draw good sell OF
	for (int i = detectDTWithGoodOF->size() - 1 ; i >= 0; i--)   
	{
	/*	// Alert	
		if(detectDTWithGoodOF->at(i).indexOfGoodOfEndBar == detectDTWithGoodOF->size()-1)
		{
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber() , "11111");
		}
		
		if(detectDTWithGoodOF->at(i).indexOfGoodOfEndBar == sc.ArraySize-1)
		{
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber() , "22222" );
		}*/
		
		if(detectDTWithGoodOF->at(i).indexOfGoodOfEndBar == sc.ArraySize-1-1)
		{
			SCString text;
			int slInTick = 1 + sc.PriceValueToTicks(sc.High[detectDTWithGoodOF->at(i).indexOfGoodOfStartBar] - sc.Low[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar]);
			int tpInTick = tp_in_ticks ;
			
			text.Format("GU SELL NOW! at %f", sc.Low[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] );
			text.Append("\n");
			text.AppendFormat("SL at %f [%dT]" , sc.High[detectDTWithGoodOF->at(i).indexOfGoodOfStartBar] + sc.TickSize, slInTick );
			text.Append("\n");
			text.AppendFormat("TP at %f [%dT]" , sc.Low[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] - (tp_in_ticks*sc.TickSize), tpInTick );
			
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber() , text );
		}
			
		//float max_high  = max(detectDTWithGoodOF->at(i).highOfPivotHigh , detectDTWithGoodOF->at(i).highOfGoodOF );
		float max_high = (detectDTWithGoodOF->at(i).highOfPivotHigh > detectDTWithGoodOF->at(i).highOfGoodOF) ? detectDTWithGoodOF->at(i).highOfPivotHigh : detectDTWithGoodOF->at(i).highOfGoodOF;
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectDTWithGoodOF->at(i).indexOfPivotHigh;	
		rectangle.EndIndex = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar;
		rectangle.BeginValue = max_high ;
		rectangle.EndValue = max_high - 3*sc.TickSize ;
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectDTWithGoodOF->at(i).lineNumberOf3tzone ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
		
		// draw good sell OF
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar;	
		rectangle.EndIndex = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar; 
		rectangle.BeginValue = detectDTWithGoodOF->at(i).highOfGoodOF ;
		rectangle.EndValue = detectDTWithGoodOF->at(i).lowOfGoodOF ;
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectDTWithGoodOF->at(i).lineNumberOfGoodOf ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
	}	
	
	
}



// Function to find pivot highs
bool iSPivotHigh(SCStudyInterfaceRef sc , int index, int pivotLength) 
{
	bool isPivotHigh = true;
	
	if(index + pivotLength >= sc.ArraySize-1 )
	{
		return false ;
	}
	
	for (int i = 1; i <= pivotLength; i++) 
	{        
       if (sc.High[index] <= sc.High[index - i] || sc.High[index] <= sc.High[index + i]) {
                isPivotHigh = false;
                break;
        }
        
    }
    return isPivotHigh;
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
						
						
											
						//auto& data = detectedGoodBuyStartBars;
						detectGoodSellOF.emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
												
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
