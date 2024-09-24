






#include "sierrachart.h"
 
 
 
#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

#define MAX_VECTOR_SIZE 100


SCDLLName("SUA ALERT DB GOOD OF")

int uniqueNumber = 343422 ;
int uniqueNumberForPivot = 86122 ;


// Structure to represent an PivotLow
struct st_PivotLow {
	int index;
	int lineNumber;
   
	st_PivotLow(int idx , int ln) : index(idx) ,lineNumber(ln) {}
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
  
	st_GoodBuyOF(int idx1, int v1 , int d1  , SCDateTime t1 ,
				 int idx2, int v2 , int d2  , SCDateTime t2 ,
				 int ln ) 
				 : startBarIndex(idx1), volume_of_start_bar(v1), delta_of_start_bar(d1) , time_of_start_bar(t1) ,
				   endBarIndex(idx2)  , volume_of_end_bar(v2)  , delta_of_end_bar(d2)   , time_of_end_bar(t2)   ,
				   lineNumber(ln) {}
	
};

// Structure to represent an good sell OF start bar
struct st_DoubleBottomWithGoodOF {
	int lineNumberOf3tzone;
	int lineNumberOfGoodOf;
	int indexOfPivotLow;
	int indexOfGoodOfStartBar;
	int indexOfGoodOfEndBar;
	float lowOfPivotLow;
	float highOfGoodOF;
	float lowOfGoodOF;
   
	st_DoubleBottomWithGoodOF(int i1, int i2 , int i3 , int i4 , int i5 ,
	                       float f1 , float f2 , float f3) 
						   : lineNumberOf3tzone(i1), lineNumberOfGoodOf(i2), indexOfPivotLow(i3) ,indexOfGoodOfStartBar(i4) , indexOfGoodOfEndBar(i5) ,
 						     lowOfPivotLow(f1) , highOfGoodOF(f2) , lowOfGoodOF(f3) {}
};




bool iSPivotLow(SCStudyInterfaceRef sc , int index, int pivotLength) ;

int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodBuyOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , 
						  std::vector<st_GoodBuyOF>& detectGoodBuyOF);
						  
float findMaxGoUpBeforeBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) ;		

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback) ;

int findHowManyTouchLow(SCStudyInterfaceRef sc , int stIndex , int edIndex);

int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num );

int cummulateDelta(SCStudyInterfaceRef sc , int index , int lookback) ;

void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString)  ;

bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )   ;
									
int findBreakLowOfPrevBar(SCStudyInterfaceRef sc ,int index , int lookback  ) ;

int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum ) ;




SCSFExport scsf_AlertDBWithGoodOF(SCStudyInterfaceRef sc)
{
	
	
	SCString msg;
	
	SCInputRef i_minBar = sc.Input[0];
	SCInputRef i_pivotLength = sc.Input[1];
	SCInputRef i_tp_in_ticks = sc.Input[2];
	SCInputRef i_special_mean = sc.Input[3];
	SCInputRef i_vol_per_sec = sc.Input[4];
	SCInputRef AlertSound = sc.Input[5];
	
	
	int BUY_SIGNAL = 1 ;
	int M1ChartNumber = 7 ;	
	int DB_GOOD_OF_CODE = 2000
	;
	SCString m1TimeString , currentTimeString;
	
	int persistentUniqueValue = sc.GetPersistentInt(5);   // 5 is key
	
	int lastProcessedBarIndex = sc.GetPersistentInt(6);
	//static int lastProcessedBarIndex = -1;
	
	
	// Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA ALERT DB WITH GOOD OF";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 2;
		
		sc.Subgraph[0].Name = "BUY SIGNAL";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_BAR;
        sc.Subgraph[0].LineWidth = 7;
		sc.Subgraph[0].PrimaryColor = RGB(255,255,0);
		
		sc.Subgraph[1].Name = "START IDX";
		sc.Subgraph[1].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[1].LineWidth = 7;
		
		sc.Subgraph[2].Name = "END IDX";
		sc.Subgraph[2].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[2].LineWidth = 7;
		
		
		
		i_pivotLength.Name = "number of bar each side | ex. 4 is left 4 bars , right 4 bars";
		i_pivotLength.SetInt(2);
		
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(30);
		
		i_minBar.Name = "min number of bar";
		i_minBar.SetInt(4);
		
		i_special_mean.Name = "spacial mean";
		i_special_mean.SetInt(3);
		
		i_vol_per_sec.Name = "max volume per sec";
		i_vol_per_sec.SetFloat(3.6);
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(2);  // Default alert sound number
		
		return;
	}
	
	
	int MaxBarLookback = 0;
	int MIN_START_INDEX = 2;   // need 3 so it is 0 1 2
	
	int minBar = i_minBar.GetInt();
	int pivotLength = i_pivotLength.GetInt();
	int tp_in_ticks = i_tp_in_ticks.GetInt();
	int spacial_mean = i_special_mean.GetInt();
	float f_vol_per_sec = i_vol_per_sec.GetFloat();
	
	auto detectPivotLows = static_cast<std::vector<st_PivotLow>*>(sc.GetPersistentPointer(2)); 
	auto detectedGoodBuyStartBars = static_cast<std::vector<st_GoodBuyOFStartBar>*>(sc.GetPersistentPointer(3));
	auto detectGoodBuyOF = static_cast<std::vector<st_GoodBuyOF>*>(sc.GetPersistentPointer(4));
	auto detectDBWithGoodOF = static_cast<std::vector<st_DoubleBottomWithGoodOF>*>(sc.GetPersistentPointer(5));
	auto fillterDBwithGoodBuyOF = static_cast<std::vector<st_DoubleBottomWithGoodOF>*>(sc.GetPersistentPointer(7));
	
	

	
	if (!detectPivotLows) {
        detectPivotLows = new std::vector<st_PivotLow>();
        sc.SetPersistentPointer(2, detectPivotLows);
		
    }	
	
	if (!detectedGoodBuyStartBars) {
        detectedGoodBuyStartBars = new std::vector<st_GoodBuyOFStartBar>();
        sc.SetPersistentPointer(3, detectedGoodBuyStartBars);
    }	
	
	if (!detectGoodBuyOF) {
        detectGoodBuyOF = new std::vector<st_GoodBuyOF>();
        sc.SetPersistentPointer(4, detectGoodBuyOF);
    }
	
	if (!detectDBWithGoodOF) {
        detectDBWithGoodOF = new std::vector<st_DoubleBottomWithGoodOF>();
        sc.SetPersistentPointer(5, detectDBWithGoodOF);
    }
	
	if (!fillterDBwithGoodBuyOF) 
	{
		fillterDBwithGoodBuyOF = new std::vector<st_DoubleBottomWithGoodOF>();
		sc.SetPersistentPointer(7, fillterDBwithGoodBuyOF);
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
		
		for (int i = 0; i < detectPivotLows->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectPivotLows->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectPivotLows->clear();
		
		for (int i = 0; i < detectGoodBuyOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodBuyOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodBuyOF->clear();
		
		for (int i = 0; i < detectDBWithGoodOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectDBWithGoodOF->at(i).lineNumberOf3tzone);
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectDBWithGoodOF->at(i).lineNumberOfGoodOf);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectDBWithGoodOF->clear();
				
		lastProcessedBarIndex = -1;	
		sc.SetPersistentInt(6, lastProcessedBarIndex);
		
		persistentUniqueValue = 0;
		sc.SetPersistentInt(5, persistentUniqueValue);		
			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	// ################################         MUST clear vector for                ###########################################
	// ################################      1. help speed up 3 times                ###########################################
	// ################################      2. protect it add same data	         ###########################################
	
	
	if (detectPivotLows != NULL)
		detectPivotLows->clear();
	
	if (detectedGoodBuyStartBars != NULL)
		detectedGoodBuyStartBars->clear();
	
	if (detectGoodBuyOF != NULL)
		detectGoodBuyOF->clear();
	
	if (detectDBWithGoodOF != NULL)
		detectDBWithGoodOF->clear();	

	if (fillterDBwithGoodBuyOF != NULL)
		fillterDBwithGoodBuyOF->clear();
	
	
	
	 // 1.Loop through bars to DB  pattern
    for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{
		
		// Check if this is a new bar
		/*if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED) {
            continue;
        }*/
		
		bool checkPivotLow = iSPivotLow(sc , i ,pivotLength );
		int ln = uniqueNumberForPivot+i ;
		
				
		// if found pivot low , then add it to vector
		if(checkPivotLow)
		{						
			// use emplace_back
			detectPivotLows->emplace_back(i ,ln );			
			
			// Ensure the vector does not exceed the maximum size
			if (detectPivotLows->size() > MAX_VECTOR_SIZE) {
				detectPivotLows->erase(detectPivotLows->begin());
			}
		
		}	
		
		//##############################                   ##############################################
		//############################## FIND GOOD BUY Start Bar ##############################################
		//##############################                   ##############################################
		
		bool isItStartBarOFGoodBuyOrderflow = checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATBOTTOM);
		
		
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
	
	
	 // 2. find end bar of good buy OF
	findGoodBuyOrderFlow(sc , *detectedGoodBuyStartBars , *detectGoodBuyOF);
	
	
	
	
	// 3. find DB with good OF
	//    loop from start of good buy OF	
	for (int i = 0 ; i < detectGoodBuyOF->size() ; i++)
	{
		int indexOfStartBar = detectGoodBuyOF->at(i).startBarIndex;
		float lowOfStartBar = sc.Low[indexOfStartBar] ;
		// find at left side of Good OF have pivot low or not
		for(int j = detectPivotLows->size() - 1 ; j >= 0 ; j--)
		{
			SCString msg;
			msg.Format("i of pv low : %d \n" ,detectPivotLows->at(j).index );
			//sc.AddMessageToLog(msg,0);
			int indexOfPivotLow = detectPivotLows->at(j).index;
			// if find index of bar of pivot low at leftside
			if( indexOfPivotLow < indexOfStartBar)
			{
				// check if low not +- more3T
				float lowOfPivot = sc.Low[detectPivotLows->at(j).index];
				int p = sc.PriceValueToTicks(lowOfPivot) -  sc.PriceValueToTicks(lowOfStartBar) ;
				if(abs(p) <= 3 )
				{
					bool noCandleBreakLow = true;
					float minValue = min(lowOfPivot, lowOfStartBar) ;
					// loop from start bar to pivot low ,then check if no candle break low of both
					for(int k = indexOfStartBar-1 ; k >= indexOfPivotLow ; k--)
					{
						
						if(sc.Low[k] < minValue)
						{
							noCandleBreakLow = false ;
							break;
						}
					}
					if(noCandleBreakLow)
					{
												
						int ln1 = detectPivotLows->at(j).lineNumber;
						int ln2 = detectGoodBuyOF->at(i).lineNumber;
						int idxPivotLow = indexOfPivotLow ;
						int idxStartBar = indexOfStartBar ;
						int idxOFEndBar = detectGoodBuyOF->at(i).endBarIndex;
						float lwOfPivot = lowOfPivot ;
						float hghOfGoodOF = sc.High[idxOFEndBar] ;
						float lowOfGoodOF = sc.Low[idxStartBar] ;
							 
						detectDBWithGoodOF->emplace_back(ln1 , ln2 ,idxPivotLow , idxStartBar , idxOFEndBar , lwOfPivot , hghOfGoodOF , lowOfGoodOF);	
						
					
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
	
	// Get min delta
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
	sc.GetStudyArrayUsingID(12, 0, vwap);		

	// for atr
	SCFloatArray ATRArray;
	sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);
			
	// for ema200
	SCFloatArray EMAArray;
	sc.GetStudyArrayFromChartUsingID(7 , 1 , 0, EMAArray);
	
	// for bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);

	// for count good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);
	
	// for good Buy OF
	SCFloatArray goodBuyArray;
	sc.GetStudyArrayUsingID(9 , 1, goodBuyArray);
	
	
	for (int i = 0; i < detectDBWithGoodOF->size(); i++)
	{
		// for get index on m5 chart
		int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, detectDBWithGoodOF->at(i).indexOfGoodOfEndBar);   // (chart number of m5 , sc.Index)
		
		SCDateTime t1 = sc.BaseDateTimeIn[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar];
		SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
		
		float hmm = findMaxGoUpBeforeBreakLow( sc , detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDBWithGoodOF->at(i).indexOfGoodOfEndBar  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		
		// for atr
		float atrValue = ATRArray[RefChartIndex];
		
		// for ema200
		float emaValue = EMAArray[RefChartIndex];

		float volum = vol[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] ;
	
		// for mean 3 /
		float mean3 = meanVol( sc , detectDBWithGoodOF->at(i).indexOfGoodOfEndBar, 7 ) ; // 3 , 50 , spacial_mean
		
		// for mean 3
		float mean_i_3 = meanVol( sc , detectDBWithGoodOF->at(i).indexOfGoodOfEndBar-7 , 7) ; //  3 , 50, spacial_mean
		
		float div_mean = mean3 / mean_i_3 ;
	
		// for mean delata
		float meanDelta5 = meanDelta(  sc , detectDBWithGoodOF->at(i).indexOfGoodOfEndBar, 5 ) ;    //5,10
		
		// for dff between low pivot and low start bar
		int diff_low_of_pv_and_start_bar = abs( sc.PriceValueToTicks( sc.Low[detectDBWithGoodOF->at(i).indexOfPivotLow] - sc.Low[detectDBWithGoodOF->at(i).indexOfGoodOfStartBar] ) );
			
		
		 //delta delta_chg max_delta min_delta percent_delta
		int idx_end_bar = detectDBWithGoodOF->at(i).indexOfGoodOfEndBar ;
		
		// it is cumulate vol 
		 int i_vol = 9;	
		float cum_vol = cummulateVol( sc , idx_end_bar ,i_vol) ;
		float cum_vol_i_10 = cummulateVol( sc , idx_end_bar-i_vol ,i_vol) ;
		float div_cum_vol = cum_vol/cum_vol_i_10 ;
	
		
	
	
		// ####################################################################################################################################
		// ################################################ Start filter DT and Good Sell OF Here  ############################################
		// ####################################################################################################################################
		
		int pvIdx = detectDBWithGoodOF->at(i).indexOfPivotLow  ;
		int startIdx = detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ;
		int endIdx = detectDBWithGoodOF->at(i).indexOfGoodOfEndBar ;
		int idxOfDBwithGoodBuy = i ;
		bool haveStartBarIsSameAsBefore = false;
		bool haveSameStartBar = false;
		bool haveSameEndBar = false;
		

		for(int j = idxOfDBwithGoodBuy-1 ; j >= 0; j-- )   // loop in good buy OF from current to 0
		{
			
			// if find start bar index same as this index before then dont want
			if( (startIdx == detectDBWithGoodOF->at(j).indexOfGoodOfStartBar)  )
			{
				haveSameStartBar = true;
			}
			
			if(haveSameStartBar  )  break;
			
		}
		
		for(int j = idxOfDBwithGoodBuy-1 ; j >= 0; j-- )   // loop in good buy OF from current to 0
		{
			
			if( (endIdx == detectDBWithGoodOF->at(j).indexOfGoodOfEndBar)  )
			{
				haveSameEndBar = true;
			}
			if(haveSameEndBar)  break;
			
		}
		
		// count good sell OF
		int cnt_good_sell = 0;
		int num = 10;  
		for(int i = startIdx-1 ; i >= startIdx-num ; i--)
		{
			if(goodSellArray[i] == 1) cnt_good_sell++;
		}
		
		
		num = 10;              //20
		int cnt_good_buy = 0;
		for(int i = startIdx-1 ; i >= startIdx-num ; i--)
		{
			if(goodBuyArray[i] == 1) cnt_good_buy++;
		}	
		
		// find how many poc at low
		int hmnPocAtLow = findHowManyDownCandleWithPocAtLow(sc ,startIdx , 9 ) ;		
		
		
		SCDateTime temp = bar_dur_array[pvIdx] ;
		float bar_sec_pv = temp.GetTimeInSeconds() ;
		
		temp = bar_dur_array[startIdx] ;
		float bar_sec_st = temp.GetTimeInSeconds() ;
		
		temp = bar_dur_array[endIdx] ;
		float bar_sec_ed = temp.GetTimeInSeconds() ;
		
		float div_vol = vol[endIdx]/ vol[endIdx-1]	;
		
		int diff_pv_gf_idx = startIdx  - pvIdx   ;
		
		int diff_st_ed_idx = endIdx - startIdx;
		
		int countPos7 = count_pos_table( sc , endIdx , 7 ) ;	
		int countPos28 = count_pos_table( sc , endIdx , 28 ) ;	//20  25  24
	
		
		if(haveSameStartBar /*|| haveSameEndBar*/ )  continue;
		if(vol[endIdx-1] < 249)   continue;
		if(t1.GetHour() < 3) continue;		       // good++++
		if(t1.GetHour() > 19) continue;	                // good++++
		if(bar_sec_pv <= 60) continue;	     	                // 80 good++++
		if(bar_sec_pv > 1100) continue;		                // good+++
		if(bar_sec_st < 40) continue;	
		if(bar_sec_ed < 60) continue;	
		if(div_vol > 2 && div_vol < 2.2 ) continue;	  
		if(diff_pv_gf_idx > 20 && div_vol < 0.2 ) continue;	  
		if(diff_st_ed_idx > 6) continue;              // good
		if(div_mean > 1.12) continue;                // good
		if(countPos7 <= 9)  continue;  
		if(countPos7 >= 27)  continue;  
		if(countPos28 >= 58 && countPos28 <= 60)  continue;     
		if(countPos28 <= 48)  continue;  
		if(countPos28 >= 92)  continue;     //   87 is good to filter 20T profit
		if(div_cum_vol > 1.1 && div_cum_vol < 1.25) continue;        // 1.1 &1.25  good
		if(vol_per_sec[endIdx] > f_vol_per_sec)  continue;			//1.4      // good
		if(vol_per_sec[endIdx] < 0.18)  continue;	
		if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= 20)  continue;		 //295	
		if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) <= -40)  continue;		 //295	
		if(cnt_good_sell >= 2) continue;	
		if(cnt_good_buy == 1) continue;	
		if(hmnPocAtLow >= 4) continue;
				
	
		// ########################################### End filter DT with Good Sell OF Here  ############################################
		
		
		


		// ####################################################################################################################################
		// ################################################ Put Signal To subgraph###############  ############################################
		// ####################################################################################################################################			
		
			
		sc.Subgraph[0][detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] = 1;	
		sc.Subgraph[1][detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] = startIdx;	
		sc.Subgraph[2][detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] = endIdx;	
	
	
		
		
		// ######################## Add data to fillterDTwithGoodSellOF Object 
		int i1 = detectDBWithGoodOF->at(i).lineNumberOf3tzone;
		int i2 = detectDBWithGoodOF->at(i).lineNumberOfGoodOf;
		int i3 = detectDBWithGoodOF->at(i).indexOfPivotLow;
		int i4 = detectDBWithGoodOF->at(i).indexOfGoodOfStartBar;
		int i5 = detectDBWithGoodOF->at(i).indexOfGoodOfEndBar;
		float f1 = detectDBWithGoodOF->at(i).lowOfPivotLow;
		float f2 = detectDBWithGoodOF->at(i).highOfGoodOF;
		float f3 = detectDBWithGoodOF->at(i).lowOfGoodOF;
		
		fillterDBwithGoodBuyOF->emplace_back(i1,i2,i3,i4,i5,f1,f2,f3);	
		

				
	
		
	
	}
	
	
	
	 // ************************* ALERT HERE ************************************
	

	int currentIndex = sc.UpdateStartIndex ; 
	//static int lastProcessedBarIndex = -1;
	
	if(currentIndex >= sc.DataStartIndex && 
		sc.Subgraph[0][currentIndex-1] == 1 && 
	    sc.GetBarHasClosedStatus(currentIndex-1)==BHCS_BAR_HAS_CLOSED )
	{
		
		
		 if ( currentIndex != lastProcessedBarIndex  ) // Only send message if it's a new bar
		{
			lastProcessedBarIndex = currentIndex; 
			sc.SetPersistentInt(6, lastProcessedBarIndex);	
			
			SCString message , textToTelegram;
		
			int idx_of_start_bar = sc.Subgraph[1][currentIndex-1] ;
			int idx_of_end_bar = sc.Subgraph[2][currentIndex-1] ;
			
			int slInTick = 1 + sc.PriceValueToTicks(sc.High[idx_of_end_bar] - sc.Low[idx_of_start_bar]);
			int tpInTick = tp_in_ticks ;
			
			int slIndex = idx_of_start_bar;
			
			persistentUniqueValue++ ;
				
			if(persistentUniqueValue > 500)
				persistentUniqueValue = 0;
				
			sc.SetPersistentInt(5, persistentUniqueValue);	
					
					
			int uniqNumber = persistentUniqueValue + DB_GOOD_OF_CODE  ;
			message.Format("BUY | index = %d , uniq = %d" , idx_of_end_bar , uniqNumber   ); // sc.CurrentSystemDateTime
			sc.AddMessageToLog(message, 0);
			
			bool isTimeCorrect = get_M1Time_currentTime_String( sc ,M1ChartNumber , slIndex ,  BUY_SIGNAL , m1TimeString , currentTimeString ) ;
					
					
			if ( /*idx_of_end_bar == sc.ArraySize - 1 - 1  &&*/ isTimeCorrect /*&& sc.GetBarHasClosedStatus(idx_of_end_bar)==BHCS_BAR_HAS_CLOSED*/) 
			{
				if (sc.IsFullRecalculation) return;
				
				sendHttpPostMessage( sc, uniqNumber ,  BUY_SIGNAL ,  m1TimeString , currentTimeString) ;
				 // Update the last bar index where the message was sent
				//sc.SetPersistentInt(6, currentIndex - 1);
				
			}				
			
			textToTelegram.Format("GU BUY NOW! at %.4f                                        ", sc.High[idx_of_end_bar] );
			textToTelegram += "\n";
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("SL at %.4f [%dT]                                        " , sc.Low[idx_of_start_bar] - sc.TickSize, slInTick );
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("TP at %.4f [%dT]                                        " , sc.High[idx_of_end_bar] + (tp_in_ticks*sc.TickSize), tpInTick );
			
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , textToTelegram );
			
		}
		
				
	}
	
	
	
	// 4. Alert , draw 3T zone , draw good buy OF
	for (int i = 0; i < detectDBWithGoodOF->size(); i++)   
	{
		
		float hmm = findMaxGoUpBeforeBreakLow( sc , detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDBWithGoodOF->at(i).indexOfGoodOfEndBar  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		//if(howMuchMove < tp_in_ticks) 	continue;
		
		
		
		SCString msg;
		//msg.Format("i of pv : %d | i of good OF : %d | low pv : %.4f | low gf : %.4f\n" ,detectDBWithGoodOF->at(i).indexOfPivotLow , detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ,
		//                                                                                 detectDBWithGoodOF->at(i).lowOfPivotLow   , detectDBWithGoodOF->at(i).lowOfGoodOF   );
		//sc.AddMessageToLog(msg,0);
	
		// draw 3T zone
		float min_low = (detectDBWithGoodOF->at(i).lowOfPivotLow < detectDBWithGoodOF->at(i).lowOfGoodOF) ? detectDBWithGoodOF->at(i).lowOfPivotLow : detectDBWithGoodOF->at(i).lowOfGoodOF;
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectDBWithGoodOF->at(i).indexOfPivotLow;	
		rectangle.EndIndex = detectDBWithGoodOF->at(i).indexOfGoodOfStartBar;
		rectangle.BeginValue = min_low ;
		rectangle.EndValue = min_low + 3*sc.TickSize ;
		rectangle.Color = RGB(0, 0, 0);
		rectangle.SecondaryColor = RGB(255, 255, 0);  // y color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectDBWithGoodOF->at(i).lineNumberOf3tzone ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
		
		// draw good buy OF
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectDBWithGoodOF->at(i).indexOfGoodOfStartBar;	
		rectangle.EndIndex = detectDBWithGoodOF->at(i).indexOfGoodOfEndBar; 
		rectangle.BeginValue = detectDBWithGoodOF->at(i).highOfGoodOF ;
		rectangle.EndValue = detectDBWithGoodOF->at(i).lowOfGoodOF ;
		rectangle.Color = RGB(0, 0, 0);
		rectangle.SecondaryColor = RGB(255, 255, 0);  // y color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectDBWithGoodOF->at(i).lineNumberOfGoodOf ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
	}
	
	
	
	
	
	
	
	
}







// Function to find pivot lows
bool iSPivotLow(SCStudyInterfaceRef sc , int index, int pivotLength) 
{
	bool isPivotLow = true;
	
	if(index + pivotLength >= sc.ArraySize-1 )
	{
		return false ;
	}
	
	for (int i = 1; i <= pivotLength; i++) 
	{        
       if (sc.Low[index] >= sc.Low[index - i] || sc.Low[index] >= sc.Low[index + i]) {
                isPivotLow = false;
                break;
        }
        
    }
    return isPivotLow;
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


void findGoodBuyOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , std::vector<st_GoodBuyOF>& detectGoodBuyOF) 
{
	int lowOfBar  ;
	int highOfBar ;
	int lowOfStartBar ;
	int highOfStartBar ;
			
    // loop from first index of start bar of good buy OF to last 
	for (const auto& bar : detectedGoodBuyStartBars) {
        int startBarIndex = bar.index;
		SCString msg;
		msg.Format("high of index %d is valid start bar | pRICE = %d  " , bar.index , sc.PriceValueToTicks(sc.High[bar.index])  );
		//sc.AddMessageToLog(msg,0);
			
		// loop from start bar to current bar
        for (int j = startBarIndex + 1; j < sc.ArraySize - 1; ++j) 
		{
			// check if start bar and end bar far apart 1 Tick ?
			lowOfBar = sc.PriceValueToTicks(sc.Low[j]) ;
			highOfBar = sc.PriceValueToTicks(sc.High[j]) ;
			lowOfStartBar = sc.PriceValueToTicks(sc.Low[startBarIndex]) ;
			highOfStartBar = sc.PriceValueToTicks(sc.High[startBarIndex]) ;			
			
			
            if((lowOfBar <= highOfStartBar + 1) && (lowOfBar >= highOfStartBar - 1))
			{
			
				// check end bar is 5 level , poc on top , up candle
                if (checkValidBar(sc, j, CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE)) 
				{
					SCString msg;
					msg.Format("found valid poc at top and up candle start bar : %d | end bar : %d \n" , startBarIndex , j );
					//sc.AddMessageToLog(msg,0);
					// check no candle go below low of start bar
					bool noCandleBreakLow = true;
					for(int k = startBarIndex ; k <= j ; ++k )
					{
						if(sc.Low[k] < sc.Low[startBarIndex])
						{
							noCandleBreakLow = false ;
							break;
						}
					}					
					// draw if no candle break low of start bar
					if(noCandleBreakLow)
					{
						int idx1 = startBarIndex;
						int v1 = sc.Volume[startBarIndex];
						int d1 = 0;
						SCDateTime t1 = sc.BaseDateTimeIn[startBarIndex];
						
						int idx2 = j ;
						int v2 = sc.Volume[j];
						int d2 = 0;
						SCDateTime t2 = sc.BaseDateTimeIn[j];	

                        int ln = uniqueNumber + startBarIndex + j ;	
						
						
											
						//auto& data = detectedGoodBuyStartBars;
						detectGoodBuyOF.emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
												
						
					}
                 
                }
            }
        }
    }
}


float findMaxGoUpBeforeBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) 
{
	//float low = sc.Low[indexOfStartBar] - 0.0001 ;     // SL at low
	//float low = sc.High[indexOfEndtBar] - 0.0005 ;
    //int high = 	sc.PriceValueToTicks(sc.Low[indexOfEndtBar]) + 5 ;// SL at 
	int sl = 	sc.PriceValueToTicks(sc.Low[indexOfStartBar]) - 1 ;// SL at 
	
	int indexThatBreakLow = -1;
	
	// find which index break low
	for(int i = indexOfEndtBar ; i < sc.ArraySize-1 ; i++)
	{
		if( sc.PriceValueToTicks(sc.Low[i]) <= sl )
		{
			indexThatBreakLow = i;
			break;
		}
	}
	
	if(indexThatBreakLow == -1)
	{
		indexThatBreakLow = sc.ArraySize-1;
	}
	
	float maxPriceGo = -100;
	
	for(int i = indexOfEndtBar ; i <= indexThatBreakLow ; i++)
	{
		if(sc.High[i] > maxPriceGo)
		{
			maxPriceGo = sc.High[i] ;
		}
	}
	
	return maxPriceGo - sc.High[indexOfEndtBar]  ;
	
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


void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString)
{
	SCString postData;
	
	postData.Format("{\"uniq\": %d, \"signal\": %d , \"m1time\": \"%s\", \"currenttime\": \"%s\"}" , 
                uniqNumber, signal, m1TimeString.GetChars(), currentTimeString.GetChars());

	const char* serverURL = "http://localhost:8000/simplesignal";
	
	
	 n_ACSIL::s_HTTPHeader HTTPHeader;
	HTTPHeader.Name = "Content-Type";
	HTTPHeader.Value = "application/json";

	if (!sc.MakeHTTPPOSTRequest(serverURL, postData.GetChars(), &HTTPHeader, 1))
	{
		sc.AddMessageToLog("Error making HTTP request.", 0);
	}
	else
	{
		sc.AddMessageToLog("HTTP request sent successfully.", 0);						
	}
	
}

bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )
{
	
	SCDateTimeArray DateTimeArray;
    sc.GetChartDateTimeArray(M1ChartNumber, DateTimeArray); 
	
	if (DateTimeArray.GetArraySize() == 0)  return false; 

	SCGraphData M1GraphData;
    // Get the base graph data from the specified chart
    sc.GetChartBaseData(M1ChartNumber, M1GraphData);

    // Define a reference to the High array
    SCFloatArrayRef HighArrayM1 = M1GraphData[SC_HIGH];
	SCFloatArrayRef LowArrayM1 = M1GraphData[SC_LOW];
	
	
	int RefChartIndex =sc.GetNearestMatchForDateTimeIndex(M1ChartNumber,IndexOfSL);
	
	float low = sc.Low[IndexOfSL] ;
	float high = sc.High[IndexOfSL] ;
	
	SCString message;	
	
	if(signal == 1)
	{
		int lowInTick = sc.PriceValueToTicks(low) ;
		int lowM1InTick = -1 ;
		
		
		for(int i = RefChartIndex ; i >=0 ; i--)
		{
			lowM1InTick = sc.PriceValueToTicks(LowArrayM1[i]) ;
			if( lowInTick ==  lowM1InTick) 
			{
				//uniqNumber += Index;	
							
				
				SCString DateTimeString = "t m1 = " ; 
				DateTimeString += sc.DateTimeToString(DateTimeArray[i],FLAG_DT_COMPLETE_DATETIME);
				
				sc.AddMessageToLog(DateTimeString, 0);
				
				int Year, Month, Day, Hour, Minute, Second;
				

				//SCString M1TimeString , currentTimeString;
				
				SCDateTime m1time = DateTimeArray[i] ;
				m1time.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);
				m1TimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);						
				
				SCDateTime currentTime = sc.CurrentSystemDateTime;
				currentTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);						
				currentTimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);
			
				return true;
				
			}
			
		}
	}
	else if(signal == -1)
	{
		
		int highInTick = sc.PriceValueToTicks(high) ;
		int highM1InTick = -1 ;
		
		for(int i = RefChartIndex ; i >=0 ; i--)
		{
			highM1InTick = sc.PriceValueToTicks(HighArrayM1[i]) ;
			if( highInTick ==  highM1InTick) 
			{
				//uniqNumber += Index;	
									
								
				SCString DateTimeString = "t m1 = " ; 
				DateTimeString += sc.DateTimeToString(DateTimeArray[i],FLAG_DT_COMPLETE_DATETIME);
				
				sc.AddMessageToLog(DateTimeString, 0);
				
				int Year, Month, Day, Hour, Minute, Second;
				

				//SCString M1TimeString , currentTimeString;
				
				SCDateTime m1time = DateTimeArray[i] ;
				m1time.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);
				m1TimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);						
				
				SCDateTime currentTime = sc.CurrentSystemDateTime;
				currentTime.GetDateTimeYMDHMS(Year, Month, Day, Hour, Minute, Second);						
				currentTimeString.Format("%04d.%02d.%02d %02d:%02d", Year, Month, Day, Hour, Minute);				
				

				return true;
			}
		}
		
		
	}
	
	return false;
	
	
	
}


int findBreakLowOfPrevBar(SCStudyInterfaceRef sc ,int index , int lookback  )
{
	int count = 0;
	int endIndex = index-lookback;
	
	if(endIndex < 0) return 1000;
	
	for(int i = index ; i >= endIndex ; i--)
	{
		if(sc.Low[i] <= sc.Low[i-1]) count++ ;
	}
	return count;
	
}


int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum )
{
	
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
	
	// Get min delta
	SCFloatArray percent_delta;
	sc.GetStudyArrayUsingID(3, 10, percent_delta);
	
	
	int count_of_positive = 0;
	
	for(int k = idx ; k > idx-lookbackColum ; k--)
	{
		if(delta[k] >= 0 ) count_of_positive++ ;
		if(delta_chg[k] >= 0 ) count_of_positive++ ;
		if(max_delta[k] >= 0 ) count_of_positive++ ;
		if(min_delta[k] >= 0 ) count_of_positive++ ;
		if(percent_delta[k] >= 0 ) count_of_positive++ ;
	}
	
	return count_of_positive;
	
}








