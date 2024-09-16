















#include "sierrachart.h"

#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

#define MAX_VECTOR_SIZE 6000


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

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback) ;

int findHowManyTouchLow(SCStudyInterfaceRef sc , int stIndex , int edIndex);

int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num );

int findHowManyUpCandleWithPocAtHigh(SCStudyInterfaceRef sc , int idx , int num ) ;

int findHowManyUpCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num );

int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num ) ;

int findHowManyTouchLowPivot(SCStudyInterfaceRef sc , int idxPV , int idxST ) ;

int cummulateDelta(SCStudyInterfaceRef sc , int index , int lookback) ;

float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;




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
		
		sc.GraphRegion = 3;
		
		sc.Subgraph[0].Name = "SELL SIGNAL";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_BAR;
        sc.Subgraph[0].LineWidth = 7;
		
		sc.Subgraph[1].Name = "START IDX";
		sc.Subgraph[1].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[1].LineWidth = 7;
		
		sc.Subgraph[2].Name = "END IDX";
		sc.Subgraph[2].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[2].LineWidth = 7;
		
		i_pivotLength.Name = "number of bar each side | ex. 4 is left 4 bars , right 4 bars";
		i_pivotLength.SetInt(2);
		
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(40);
		
	
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(3);  // Default alert sound number		
		
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
	auto fillterDTwithGoodSellOF = static_cast<std::vector<st_DoubleTopWithGoodOF>*>(sc.GetPersistentPointer(7));
	
	
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
	
	if (!fillterDTwithGoodSellOF) 
	{
		fillterDTwithGoodSellOF = new std::vector<st_DoubleTopWithGoodOF>();
		sc.SetPersistentPointer(7, fillterDTwithGoodSellOF);
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
	
	if (fillterDTwithGoodSellOF != NULL)
		fillterDTwithGoodSellOF->clear();
	
	
	
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
			
		}		
		
	}
	
	/*int currentIndex = sc.UpdateStartIndex ;
	
	if(currentIndex >= sc.DataStartIndex && sc.Subgraph[0][currentIndex-1] == 1)
		sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , "good 8 sell start bar" );*/
	
	
	
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
						
						//sc.Subgraph[0][detectGoodSellOF->at(i).endBarIndex] = 1;
						
					}
					else
					{						
						//sc.Subgraph[0][detectGoodSellOF->at(i).endBarIndex] = 0;
												
					}
						
				}
			}
			
		}
	}
	
	
	
	
	// Get the study ID3 and SG1 (=0)    | it is delta
	SCFloatArray delta;
	sc.GetStudyArrayUsingID(3, 0, delta);
	
	// Get delta chg
	SCFloatArray delta_chg;
	sc.GetStudyArrayUsingID(3, 2, delta_chg);
	
	// Get max delta
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta);       
	
	// Get min delta
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);
	
	// Get percent delta
	SCFloatArray percent_delta;
	sc.GetStudyArrayUsingID(3, 10, percent_delta);
	
	// it is cum delta low of start bar
	SCFloatArray cum_delta_low;
	sc.GetStudyArrayUsingID(3, 21, cum_delta_low);
	
	// it is totl vol
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);
	
	// it is vol/sec
	SCFloatArray vol_per_sec;
	sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
	
	// it is cvd
	SCFloatArray cum_vol_day;
	sc.GetStudyArrayUsingID(3, 39, cum_vol_day);
	
	
	
	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(13, 0, vwap);		

	// for atr
	SCFloatArray ATRArray;
	sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);
			
	// for ema200
	SCFloatArray EMAArray;
	sc.GetStudyArrayFromChartUsingID(7 , 1 , 0, EMAArray);
	
	// it is bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
	
	// for count good buy OF
	SCFloatArray goodBuyArray;
	sc.GetStudyArrayUsingID(9 , 1, goodBuyArray);
	
	// for count good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);
	
	// for good Sell st index 
	SCFloatArray goodSellStartIdxArray;
	sc.GetStudyArrayUsingID(9 , 4, goodSellStartIdxArray);
		
		
		
	// put signal in subgraph
	for (int i = detectDTWithGoodOF->size() - 1 ; i >= 0; i--)   
	{
							
		
		int pvIdx = detectDTWithGoodOF->at(i).indexOfPivotHigh ;
		int stIdx = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar ;
		int edIdx = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar ;
		
		SCDateTime t1 = sc.BaseDateTimeIn[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar];
		SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
		
		int dff_st_ed_idx = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar - detectDTWithGoodOF->at(i).indexOfGoodOfStartBar  ;
		int diff_pv_gf_idx = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar  - detectDTWithGoodOF->at(i).indexOfPivotHigh  ;

		
	
		// for mean 5 last 5 vol , if idx = 10 , i = 9,8,7,6,5
		float mean3 = meanVol( sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar, 7 ) ; // 3,5
		
		// for mean 5 last 10 vol , if idx = 10 , i = 4,3,2,1,0
		float mean_i_3 = meanVol( sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar-7 , 7) ; // 3,5
	
		// for mean delata
		float meanDelta5 = meanDelta(  sc , detectDTWithGoodOF->at(i).indexOfGoodOfEndBar, 5 ) ;    //5,10

		SCDateTime  bar_time = bar_dur_array[edIdx] ; 
		float bar_sec = bar_time.GetTimeInSeconds(); 		
		
		// it is bar duration pivot
		SCDateTime temp = bar_dur_array[pvIdx] ;
		float bar_sec_pv = temp.GetTimeInSeconds() ;
		
		int i_5 = 5;
		float meanMinDel = meanMinDelta( sc , edIdx, i_5 ) ;
		float meanMinDelta5_i_5 = meanMinDelta( sc , edIdx-i_5 ,i_5 ) ;
		float div_mean_min_del = meanMinDel/meanMinDelta5_i_5 ;
		
		int num_up_candle_poc_at_high = findHowManyUpCandleWithPocAtHigh( sc , edIdx , 9 );
		
		 //delta delta_chg max_delta min_delta percent_delta
		int count_of_positive = 0;
		for(int k = edIdx ; k > edIdx-7 ; k--)   //endIndex
		{
			if(delta[k] > 0 ) count_of_positive++ ;
			if(delta_chg[k] > 0 ) count_of_positive++ ;
			if(max_delta[k] > 0 ) count_of_positive++ ;
			if(min_delta[k] > 0 ) count_of_positive++ ;
			if(percent_delta[k] > 0 ) count_of_positive++ ;
		}
		
		float div_vol_ed_st_idx = vol[edIdx] / vol[stIdx]  ;   
		
		int num_touch_vwap = findHowManyTouchVwap( sc , edIdx , 50 );   //30
		
		int hmnTouchPV = findHowManyTouchLowPivot( sc ,pvIdx , stIdx ) ;
		
		// count good sell OF
		int num = 10;
		int cnt_good_sell = 0;
		int diff_high_of_good_st_bar = 999 ;
		for(int i = edIdx-1 ; i >= edIdx-num ; i--)
		{
			if(goodSellArray[i] == 1 && goodSellStartIdxArray[i] != pvIdx  
				&& stIdx > goodSellStartIdxArray[i] /*&& pivotIndex > goodSellStartIdxArray[i]*/) 
			{
				cnt_good_sell++;
				diff_high_of_good_st_bar = sc.PriceValueToTicks( sc.High[stIdx] ) - sc.PriceValueToTicks(sc.High[goodSellStartIdxArray[i]] )  ;
				break ;
			}
		}
		
		// count good buy OF
		num = 5;
		int cnt_good_buy = 0;		 
		for(int i = edIdx-1 ; i >= edIdx-num ; i--)    // startIndex
		{
			if(goodBuyArray[i] == 1) cnt_good_buy++;
		}
			
		
		
		if(vol[edIdx-1] < 90) continue;
		if(cnt_good_buy >= 1) continue;
		if(cnt_good_sell >= 1 && diff_high_of_good_st_bar >= 3) continue;	
		if(dff_st_ed_idx >= 6 && dff_st_ed_idx <= 7) continue;	
		if(dff_st_ed_idx >= 18) continue;
		if(diff_pv_gf_idx < 3) continue;	
		if(diff_pv_gf_idx > 40) continue;
		if(dff_st_ed_idx == 1 && div_vol_ed_st_idx < 0.8 && div_vol_ed_st_idx > 0.1 )  continue;	
		if(div_vol_ed_st_idx > 12 )  continue;	    // for diff st and ed index more than 1
		if (bar_sec > 2000) continue;
		if (bar_sec_pv > 2200) continue; 
		if(num_up_candle_poc_at_high >= 3)  continue;  
		if(count_of_positive <= 12)  continue;   
		if(num_touch_vwap >= 10)  continue; 
		if(div_mean_min_del > 4) continue;	
		if(hmnTouchPV >= 8) continue;
		if(vol_per_sec[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] < 0.13)  continue;				
		if(vol_per_sec[detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] > 1.78)  continue;			//1.4
		if(sc.PriceValueToTicks( vwap[edIdx]-sc.Close[edIdx] ) >= 6)  continue;
		 if( i >=1 && pvIdx == detectDTWithGoodOF->at(i-1).indexOfPivotHigh && stIdx == detectDTWithGoodOF->at(i-1).indexOfGoodOfStartBar ) continue ;
		 // -------------  go next i if have same end index
		//if( i >=1 && edIdx == detectDTWithGoodOF->at(i-1).indexOfGoodOfEndBar) continue ;  // good
		
		
		
		// ########################################### End filter DT with Good Sell OF Here  ############################################
		
		
		
		sc.Subgraph[0][detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] = 1;	
		sc.Subgraph[1][detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] = stIdx;	
		sc.Subgraph[2][detectDTWithGoodOF->at(i).indexOfGoodOfEndBar] = edIdx;	
			
		// ######################## Add data to fillterDTwithGoodSellOF Object 
		int i1 = detectDTWithGoodOF->at(i).lineNumberOf3tzone;
		int i2 = detectDTWithGoodOF->at(i).lineNumberOfGoodOf;
		int i3 = detectDTWithGoodOF->at(i).indexOfPivotHigh;
		int i4 = detectDTWithGoodOF->at(i).indexOfGoodOfStartBar;
		int i5 = detectDTWithGoodOF->at(i).indexOfGoodOfEndBar;
		float f1 = detectDTWithGoodOF->at(i).highOfPivotHigh;
		float f2 = detectDTWithGoodOF->at(i).highOfGoodOF;
		float f3 = detectDTWithGoodOF->at(i).lowOfGoodOF;
		
		fillterDTwithGoodSellOF->emplace_back(i1,i2,i3,i4,i5,f1,f2,f3);	
			
			
	}
	
	
	
	
	 // ************************* ALERT HERE ************************************
	
	int oo = 	 fillterDTwithGoodSellOF->size() - 1 ; 
	int currentIndex = sc.UpdateStartIndex ;

	if(currentIndex >= sc.DataStartIndex && sc.Subgraph[0][currentIndex-1] == 1)
	{
		SCString text;
		
		// idx_of_start_bar = fillterDTwithGoodSellOF->at(oo).indexOfGoodOfStartBar ;		
		//int idx_of_end_bar fillterDTwithGoodSellOF->at(oo).indexOfGoodOfEndBar;
		
		int idx_of_start_bar = sc.Subgraph[1][currentIndex-1] ;
		int idx_of_end_bar = sc.Subgraph[2][currentIndex-1] ;
		
		int slInTick = 1 + sc.PriceValueToTicks(sc.High[idx_of_start_bar] - sc.Low[idx_of_end_bar]);
		int tpInTick = tp_in_ticks ;
		
		text.Format("GU SELL NOW! at %.4f                                        ", sc.Low[idx_of_end_bar] );
		text += "\n";
		text.Append("\n");
		text.AppendFormat("SL at %.4f [%dT]                                        " , sc.High[idx_of_start_bar] + sc.TickSize, slInTick );
		text.Append("\n");
		text.AppendFormat("TP at %.4f [%dT]                                        " , sc.Low[idx_of_end_bar] - (tp_in_ticks*sc.TickSize), tpInTick );
		
		 // Trigger the alert
		sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , text );
		
		
	}
		
		
	for (int i = fillterDTwithGoodSellOF->size() - 1 ; i >= 0; i--)        //   fillterDTwithGoodSellOF
	{
		
	}
	
	
	
	// 4. Alert , draw 3T zone , draw good sell OF
	for (int i = detectDTWithGoodOF->size() - 1 ; i >= 0; i--)        //   fillterDTwithGoodSellOF
	{

			
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
		
	SCFloatArray poc_value;
	sc.GetStudyArrayUsingID(3, 41, poc_value);
	float poc_price = poc_value[idx];
	int pocLev = sc.PriceValueToTicks( poc_price - sc.Low[idx] ) ;

	/*SCString msg2;
	msg2.Format("------------- index %d is valid start bar | poc level = %d | lowest vol = %f | " , idx , pocLev , sc.Low[idx] );
	sc.AddMessageToLog(msg2,0);	*/
		
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
				
		
		
		//if(levelOFHighestVol <= 1) 
		if(pocLev <= 1)
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
        
		
		if(pocLev <= 1)	
			return true;
		else 
			return false;
			    
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
	   		
				
		
		//if(levelOFHighestVol >= 3) return true;
		if(pocLev >= 3) return true;
		else return false;
		
	}
	else if(condition == CHECK_5LEVEL_AND_POCATTOP)
	{
		// check is 5 level ?
		if(numPriceLevel == 5);
			//return true;
		else
			return false;
		
				
		//if(levelOFHighestVol >= 3)
		if(pocLev >= 3)		
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
	    
	   	 
		
				
	   
		//if(levelOFHighestVol <= 1) 
		if(pocLev <= 1)	
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



float meanVol( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	
	float sum=0;
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
	{
		sum += vol[i];
	}
	return sum/lookback ;
}

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray delta;
	sc.GetStudyArrayUsingID(3, 0, delta);	
	float sum=0;
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
	{
		sum += abs( delta[i] );
	}
	return sum/lookback ;
}


		
float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	// Get min delta
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);
	float sum=0;
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
	{
		sum +=  min_delta[i] ;
	}
	return sum/lookback ;
}

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	
	// Get max delta
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta); 
	float sum=0;
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
	{
		sum +=  max_delta[i] ;
	}
	return sum/lookback ;
}


int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback)
{
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += vol[i];
	}
	return sum;
	
}



int findHowManyTouchLow(SCStudyInterfaceRef sc , int stIndex , int edIndex)
{
	int cnt=0;
	float low = sc.Low[stIndex] ;
	float min_low = (sc.Low[stIndex] < sc.Low[edIndex]) ? sc.Low[stIndex] : sc.Low[edIndex];
	
	for(int i = stIndex+1 ; i < edIndex ; i++)
	//for(int i = stIndex-10 ; i < stIndex ; i++)
	{
		if(sc.Low[i] <= min_low +3*sc.TickSize ) cnt++;
	}
	return cnt;
}


int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE ) ) cnt++;
	}
	return cnt;
	
}


int findHowManyUpCandleWithPocAtHigh(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE ) ) cnt++;
	}
	return cnt;
	
}

int findHowManyUpCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE ) ) cnt++;
	}
	return cnt;
	
}


int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num )
{
	
	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(13, 0, vwap);	
		
		
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		int i_vwap = sc.PriceValueToTicks( vwap[i] ) ;
		int i_high = sc.PriceValueToTicks( sc.High[i]  ) ;
		int i_low = sc.PriceValueToTicks( sc.Low[i]  );
		
		if(i_vwap >= i_low && i_vwap <= i_high)   cnt++;
		
		
	}
	return cnt;
	
}

int findHowManyTouchLowPivot(SCStudyInterfaceRef sc , int idxPV , int idxST )
{
			
		
	int cnt=0;
	int lowPV = sc.PriceValueToTicks(sc.Low[idxPV]) ;
	
	for(int i = idxPV+1 ; i <= idxST-1 ; i++)
	{
		int highI = sc.PriceValueToTicks(sc.High[i]) ;
		
		if(highI >= lowPV) cnt++;
					
	}
	return cnt;
	
}


int cummulateDelta(SCStudyInterfaceRef sc , int index , int lookback)
{
	SCFloatArray del;
	sc.GetStudyArrayUsingID(3, 0, del);	
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += del[i];
	}
	return sum;
	
}






