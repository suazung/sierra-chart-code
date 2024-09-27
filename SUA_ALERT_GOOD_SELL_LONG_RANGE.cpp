






//   **********************************   if div cum vol < 0.61 then use SL at start bar     *************************************


#include "sierrachart.h"
#include <vector>            // For using std::vector
#include <sstream>           // For string manipulation
#include <iomanip>


#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6


#define MAX_VECTOR_SIZE 600



#define TP_ON_TICKS 40
#define SL_ON_TICKS 5

int uniqueNumber = 64444 ;


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




int isThisStartBarOFGoodSellOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodSellOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , 
						  std::vector<st_GoodSellOF>& detectGoodSellOF);						  

//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;

float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) ;

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );

float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback );

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback) ;

int findHowManyUpCandleWithPocAtHigh(SCStudyInterfaceRef sc , int idx , int num ) ;

int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num ) ;

int countPositiveTable(SCStudyInterfaceRef sc , int idx , int lookback ) ;

int findBreakHighOfPrevBar(SCStudyInterfaceRef sc ,int index , int lookback  )   ;

int numOfBarThatNot5level(SCStudyInterfaceRef sc ,int index , int lookback  )    ;

int numTouchHighOfEndIndex(SCStudyInterfaceRef sc ,int index , int lookback  )   ;

void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString) ;

bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )  ;
									
									
									


SCDLLName("SUA ALERT GOOD SELL LONG RENGE")



SCSFExport scsf_AlertGoodSellLongRenge(SCStudyInterfaceRef sc)
{
	
	int count = 0;
	SCString msg;
	
	SCInputRef i_tp_in_ticks = sc.Input[count++];
	SCInputRef AlertSound = sc.Input[count++];
	
	int SELL_SIGNAL = -1 ;
	int M1ChartNumber = 7 ;	
	int GOOD_SELL_LONG_RANGE_CODE = 4000
	;
	SCString m1TimeString , currentTimeString;
	
	int persistentUniqueValue = sc.GetPersistentInt(5);   // 5 is key
	
	int lastProcessedBarIndex = sc.GetPersistentInt(6);
	
	// Section 1 - Set the configuration variables and defaults
	
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA ALERT GOOD Sell Long Range";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 

		sc.GraphRegion = 3;
		
		sc.Subgraph[0].Name = "SELL SIGNAL";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_STAR;
        sc.Subgraph[0].LineWidth = 7;
		sc.Subgraph[0].PrimaryColor = RGB (255, 165, 0);
		
		sc.Subgraph[1].Name = "START IDX";
		sc.Subgraph[1].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[1].LineWidth = 7;
		
		sc.Subgraph[2].Name = "END IDX";
		sc.Subgraph[2].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[2].LineWidth = 7;
	
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(30);	
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(3);  // Default alert sound number		
		
		sc.AlertOnlyOncePerBar = true;
		sc.ResetAlertOnNewBar = true; 
		
		return;
	}
	
	
	
	// Section 2 - Do data processing here
	
	
	
	int tp_in_ticks = i_tp_in_ticks.GetInt();
	
	
	int MIN_START_INDEX = 0;
	int MaxBarLookback = 0;


	// See if we are capping max bars to check back
	if (MaxBarLookback == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;

	



	auto detectedGoodSellStartBars = static_cast<std::vector<st_GoodSellOFStartBar>*>(sc.GetPersistentPointer(1));
	auto detectGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(2));
	auto fillterGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(4));
	
	
	
	
	
	if (!detectedGoodSellStartBars) {
        detectedGoodSellStartBars = new std::vector<st_GoodSellOFStartBar>();
        sc.SetPersistentPointer(1, detectedGoodSellStartBars);
    }

	

	if (!detectGoodSellOF) {
        detectGoodSellOF = new std::vector<st_GoodSellOF>();
        sc.SetPersistentPointer(2, detectGoodSellOF);
    }
	
	

	if (!fillterGoodSellOF) 
	{
		fillterGoodSellOF = new std::vector<st_GoodSellOF>();
		sc.SetPersistentPointer(4, fillterGoodSellOF);
	}		
	


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
		
		
		
		lastProcessedBarIndex = -1;	
		sc.SetPersistentInt(6, lastProcessedBarIndex);
		
		persistentUniqueValue = 0;
		sc.SetPersistentInt(5, persistentUniqueValue);	


		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}


	// Clear out structure to avoid contually adding onto it and causing memory/performance issues
	// TODO: Revist above logic. For now though we can't clear it until user type drawings are removed
	// first so have to do it after that point
	if (detectedGoodSellStartBars != NULL)
		detectedGoodSellStartBars->clear();


	if (detectGoodSellOF != NULL)
		detectGoodSellOF->clear();

	
	if (fillterGoodSellOF != NULL)
		fillterGoodSellOF->clear() ;
	
	
	
	
	 // 1.Loop through bars to detect good buy start bar pattern
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
	
	

	// 2. find end bar of good sell OF
	findGoodSellOrderFlow(sc , *detectedGoodSellStartBars , *detectGoodSellOF);
	
	
	
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
		
	// it is atr 	
	SCFloatArray ATRArray;
	sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);	
		
	// it is ema 		
	SCFloatArray EMAArray;
	sc.GetStudyArrayFromChartUsingID(7 , 1 , 0, EMAArray);
	
	// for bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
	
	// for count good buy OF
	SCFloatArray goodBuyArray;
	sc.GetStudyArrayUsingID(9 , 1, goodBuyArray);
	
	// for count good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);
	
	
		
	for (int i = 0; i < detectGoodSellOF->size(); i++)
	{
		
		SCString date = sc.DateTimeToString( detectGoodSellOF->at(i).time_of_end_bar, FLAG_DT_COMPLETE_DATETIME);
		float howMuchMove = findMaxGoDownBeforeBreakHigh(sc,detectGoodSellOF->at(i).startBarIndex , detectGoodSellOF->at(i).endBarIndex) ;
		int hmm = sc.PriceValueToTicks(howMuchMove) ;

		int startIdx = detectGoodSellOF->at(i).startBarIndex ;
		int endIdx = detectGoodSellOF->at(i).endBarIndex ;
		
		
		
		
		// for atr
		int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, detectGoodSellOF->at(i).endBarIndex);   // (chart number of m5 , sc.Index)
		
		float atrValue = ATRArray[RefChartIndex];
		
		// for ema200
		
		float emaValue = EMAArray[RefChartIndex];
		
		// for mean vol
		float mean5 = meanVol( sc , detectGoodSellOF->at(i).endBarIndex, 5 ) ; //5
		
		// for mean delata
		float meanDelta5 = meanDelta(  sc , detectGoodSellOF->at(i).endBarIndex, 5 ) ;    //5,10
		
		// for mean min delta
		float meanMinDelta10 = meanMinDelta( sc , detectGoodSellOF->at(i).endBarIndex, 10  );
		
		// for mean max delta
		float meanMaxDelta10 = meanMaxDelta( sc , detectGoodSellOF->at(i).endBarIndex, 10 ) ;
		
		SCDateTime temp = bar_dur_array[endIdx] ;
		float bar_sec_ed = temp.GetTimeInSeconds() ;
		
		temp = bar_dur_array[startIdx] ;
		float bar_sec_st = temp.GetTimeInSeconds() 		;
	
		int num_up_candle_poc_at_high = findHowManyUpCandleWithPocAtHigh( sc , endIdx , 9 );
		
		
		int count_of_positive_7 = countPositiveTable( sc , endIdx , 7 )  ;
		
		int count_of_positive_2 = countPositiveTable( sc , endIdx , 2 )  ;
		
		int count_of_positive_3 = countPositiveTable( sc , endIdx , 3 )   ;
		
		
		// count good sell OF
		int num = 10;
		int cnt_good_sell = 0;
		int diff_high_of_good_st_bar = 999 ;
		for(int i = endIdx-1 ; i >= endIdx-num ; i--)
		{
			if(goodSellArray[i] == 1) 
			{
				cnt_good_sell++;
				//diff_high_of_good_st_bar = sc.PriceValueToTicks( sc.High[startIdx] ) - sc.PriceValueToTicks(sc.High[goodSellStartIdxArray[i]] )  ;
				break ;
			}
		}
		
		// count good buy OF
		num = 5;
		int cnt_good_buy = 0;		 
		for(int i = endIdx-1 ; i >= endIdx-num ; i--)    // startIndex
		{
			if(goodBuyArray[i] == 1) cnt_good_buy++;
		}
		
		int i_vol = 9;	
		float cum_vol = cummulateVol( sc , endIdx ,i_vol) ;
		float cum_vol_i_10 = cummulateVol( sc , endIdx-i_vol ,i_vol) ;
		float div_cum_vol = cum_vol/cum_vol_i_10 ;
	
		float div_vol_st_ed_idx = vol[startIdx]/vol[endIdx] ;
		
		float div_vol_ed_prev_idx = vol[endIdx]/vol[endIdx-1] ;
		
		int num_touch_vwap_last_8 = findHowManyTouchVwap( sc , endIdx , 8 );   //30  , 8
		
		int num_touch_vwap_last_50 = findHowManyTouchVwap( sc , endIdx , 50 );   //30  , 8
		
		int i_5 = 3;
		float meanMinDel = meanMinDelta( sc , endIdx, i_5 ) ;
		float meanMinDelta5_i_5 = meanMinDelta( sc , endIdx-i_5 ,i_5 ) ;
		float div_mean_min_del = meanMinDel/meanMinDelta5_i_5 ;
		//if(sumOFTouchHighOfStartIndex > 5 )  continue;  //3
	
		int brkHigh = findBreakHighOfPrevBar( sc ,endIdx, 40 );
		
		int not5Level = numOfBarThatNot5level( sc ,endIdx, 5 )  ;
		
	
		// ################################################ Start filter Good Sell OF Here  ############################################
		
					
		if(detectGoodSellOF->at(i).time_of_end_bar.GetHour() > 18) continue;	
		if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex < 19 ) continue;	//80	
		if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex > 420 ) continue;
		if(bar_sec_st < 20)  continue;
		if(bar_sec_st > 600)  continue;
		if(bar_sec_ed < 4)  continue;
		if(bar_sec_ed > 720)  continue;
		if(num_up_candle_poc_at_high > 5)  continue;
		if(count_of_positive_7 < 14)  continue;
		if(count_of_positive_7 > 20)  continue;
		if(count_of_positive_3 >= 4 && count_of_positive_3 <= 5)  continue;  
		if(count_of_positive_3 >= 12 )  continue; 		
		if(not5Level >= 1 )  continue; 		
		if(div_cum_vol > 2)  continue;
		if(div_vol_st_ed_idx < 0.18)  continue;  
		if(div_vol_st_ed_idx > 12)  continue; 
		if(div_vol_ed_prev_idx < 0.5 && vol[endIdx] > 18)  continue;   			
		if(num_touch_vwap_last_8 > 1)  continue; 
		if(num_touch_vwap_last_50 > 15)  continue;
		if(div_mean_min_del > 6)  continue;  // //5
		if(vol[endIdx-1] < 80)  continue;		
		if(brkHigh >= 30 )    continue;		
		if(vol_per_sec[endIdx] < 0.52 )  continue;			//1.4
		if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= 9)  continue;
		if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= -9 && sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) <= 8)  continue;
		
		
		int idxOfGoodSellOF = i ;
		bool haveStartBarIsSameAsBefore = false;
		float lowOfThisEndBar = sc.Low[endIdx] ;
		float volOfThisEndBar = detectGoodSellOF->at(i).volume_of_end_bar ;
		int sumOFSameStartIndex = 0;
		for(int j = idxOfGoodSellOF-1 ; j >= 0; j-- )   // loop in good buy OF from current to 0
		{
			
			// if find start bar index same as this index before then dont want
			if( (startIdx == detectGoodSellOF->at(j).startBarIndex)  )
			{
				
				if( lowOfThisEndBar < sc.Low[detectGoodSellOF->at(j).endBarIndex]  ) continue;  // if low of this end bar below low of previous end bar then it not same
				if(volOfThisEndBar < 50*detectGoodSellOF->at(j).volume_of_end_bar) continue; // if volOfThisEndBar more 10x of previous same start bar then it not same
				//if( detectGoodSellOF->at(j).volume_of_end_bar <= 20)   sumOFSameStartIndex++ ;
				
				sumOFSameStartIndex++ ;
			}
		}
		//if(sumOFSameStartIndex > 0 )  continue;
		int sumOFTouchHighOfStartIndex = 0;
		for(int k = endIdx-1 ; k >= startIdx+1; k--)
		{
			if(sc.Low[k] <= sc.High[startIdx]+sc.TickSize )   // if touch high of start bar index
			{
				sumOFTouchHighOfStartIndex++;
			}
		}
		
		
		
		
		// ########################################### End filter Good Buy OF Here  ############################################
		
		sc.Subgraph[0][endIdx] = 1;	
		sc.Subgraph[1][endIdx] = startIdx;	
		sc.Subgraph[2][endIdx] = endIdx;	  		
		
		

		// ######################## Add data to fillterGoodBuyOF Object 
		int idx1 = detectGoodSellOF->at(i).startBarIndex;
		int v1 = detectGoodSellOF->at(i).volume_of_start_bar;
		int d1 = detectGoodSellOF->at(i).delta_of_start_bar;
		SCDateTime t1 = detectGoodSellOF->at(i).time_of_start_bar;			
		int idx2 = detectGoodSellOF->at(i).endBarIndex ;
		int v2 = detectGoodSellOF->at(i).volume_of_end_bar;
		int d2 = detectGoodSellOF->at(i).delta_of_end_bar;
		SCDateTime t2 = detectGoodSellOF->at(i).time_of_end_bar;	
		int ln = detectGoodSellOF->at(i).lineNumber ;						
		fillterGoodSellOF->emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
			
	
		
	}

	
	
	 // ************************* ALERT HERE ************************************
	
	int currentIndex = sc.UpdateStartIndex ; 
	//static int lastProcessedBarIndex = -1;
	
	if(currentIndex >= sc.DataStartIndex && sc.Subgraph[0][currentIndex-1] == 1 && 
	   sc.GetBarHasClosedStatus(currentIndex-1)==BHCS_BAR_HAS_CLOSED )
	{
		
		
		 if ( currentIndex != lastProcessedBarIndex  ) // Only send message if it's a new bar
		{
			lastProcessedBarIndex = currentIndex; 
			sc.SetPersistentInt(6, lastProcessedBarIndex);	
			
			SCString message , textToTelegram;
		
			int idx_of_start_bar = sc.Subgraph[1][currentIndex-1] ;
			int idx_of_end_bar = sc.Subgraph[2][currentIndex-1] ;
			
			int slIndex = idx_of_end_bar;
			
			int i_vol = 9;	
			float cum_vol = cummulateVol( sc , idx_of_end_bar ,i_vol) ;
			float cum_vol_i_10 = cummulateVol( sc , idx_of_end_bar-i_vol ,i_vol) ;
			float div_cum_vol = cum_vol/cum_vol_i_10 ;
			float stopLoss ;
						
			if(div_cum_vol < 0.61)
			{
				stopLoss = sc.High[idx_of_start_bar] + sc.TickSize;
				slIndex = idx_of_start_bar;
			}						
			else
			{
				stopLoss = sc.High[idx_of_end_bar] + sc.TickSize ;
				slIndex = idx_of_end_bar;
			}
				
			
			int slInTick = sc.PriceValueToTicks(stopLoss - sc.Low[idx_of_end_bar]);
			
			int tpInTick = tp_in_ticks ;
			
			
			
			
			
			persistentUniqueValue++ ;
				
			if(persistentUniqueValue > 500)
				persistentUniqueValue = 0;
				
			sc.SetPersistentInt(5, persistentUniqueValue);	
					
					
			int uniqNumber = persistentUniqueValue + GOOD_SELL_LONG_RANGE_CODE  ;
			message.Format("SELL | index = %d , uniq = %d" , idx_of_end_bar , uniqNumber   ); // sc.CurrentSystemDateTime
			sc.AddMessageToLog(message, 0);
			
			bool isTimeCorrect = get_M1Time_currentTime_String( sc ,M1ChartNumber , slIndex ,  SELL_SIGNAL , m1TimeString , currentTimeString ) ;
					
					
			if ( /*idx_of_end_bar == sc.ArraySize - 1 - 1  &&*/ isTimeCorrect /*&& sc.GetBarHasClosedStatus(idx_of_end_bar)==BHCS_BAR_HAS_CLOSED*/) 
			{
				if (sc.IsFullRecalculation) return;
				
				sendHttpPostMessage( sc, uniqNumber ,  SELL_SIGNAL ,  m1TimeString , currentTimeString) ;
				 // Update the last bar index where the message was sent
				//sc.SetPersistentInt(6, currentIndex - 1);
				
			}			


									
			if(div_cum_vol < 0.61)   
				stopLoss = sc.High[idx_of_start_bar] + sc.TickSize;
			else
				stopLoss = sc.High[idx_of_end_bar] + sc.TickSize ;
			
	
			
			textToTelegram.Format("GU SELL NOW! at %.4f                                        ", sc.Low[idx_of_end_bar] );
			textToTelegram += "\n";
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("SL at %.4f [%dT]                                        " , stopLoss , slInTick );
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("TP at %.4f [%dT]                                        " , sc.Low[idx_of_end_bar] - (tp_in_ticks*sc.TickSize), tpInTick );
			
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , textToTelegram );
			
		}
		
		
		
		
	}
	
	
	
	
	
	// 3. draw rectangle of good sell OF
	for (int i = 0; i < detectGoodSellOF->size(); i++)
	{
		
		
		// **************************** Draw GOOD SELL OF HERE *********************************************
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectGoodSellOF->at(i).startBarIndex;
		rectangle.EndIndex = detectGoodSellOF->at(i).endBarIndex;
		rectangle.BeginValue = sc.High[detectGoodSellOF->at(i).startBarIndex];
		rectangle.EndValue = sc.Low[detectGoodSellOF->at(i).endBarIndex];
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 90;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectGoodSellOF->at(i).lineNumber ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
						
		SCString msg;
		msg.Format("start bar : %d | end bar : %d \n" , detectGoodSellOF->at(i).startBarIndex , detectGoodSellOF->at(i).endBarIndex );
		//sc.AddMessageToLog(msg,0);
		
		
		
		
	}
	
	
	
	
	
	
	
	
	
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


void findGoodSellOrderFlow(SCStudyInterfaceRef sc , std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , std::vector<st_GoodSellOF>& detectGoodSellOF) 
{
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
			int lowOfBar = sc.PriceValueToTicks(sc.Low[j]) ;
			int highOfBar = sc.PriceValueToTicks(sc.High[j]) ;
			int lowOfStartBar = sc.PriceValueToTicks(sc.Low[startBarIndex]) ;
			int highOfStartBar = sc.PriceValueToTicks(sc.High[startBarIndex]) ;
			
			
			
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

                        int ln = uniqueNumber + startBarIndex + j ;	
						
						
											
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

float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) 
{
	//float low = sc.Low[indexOfStartBar] - 0.0001 ;     // SL at low
	//float low = sc.High[indexOfEndtBar] - 0.0005 ;
    //int high = 	sc.PriceValueToTicks(sc.Low[indexOfEndtBar]) + SL_ON_TICKS ;// SL at 
	int high = 	sc.PriceValueToTicks(sc.High[indexOfEndtBar]) + 1 ;// SL at 
	
	int indexThatBreakHigh = -1;
	
	// find which index break high
	for(int i = indexOfEndtBar ; i < sc.ArraySize-1 ; i++)
	{
		if( sc.PriceValueToTicks(sc.High[i]) >= high )
		{
			indexThatBreakHigh = i;
			break;
		}
	}
	
	if(indexThatBreakHigh == -1)
	{
		indexThatBreakHigh = sc.ArraySize-1;
	}
	
	float maxPriceGo = 100;
	
	for(int i = indexOfEndtBar ; i <= indexThatBreakHigh ; i++)
	{
		if(sc.Low[i] < maxPriceGo)
		{
			maxPriceGo = sc.Low[i] ;
		}
	}
	
	return sc.Low[indexOfEndtBar]-maxPriceGo  ;
	
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
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);	
	float sum=0;

	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += min_delta[i];
	}
	return sum/lookback ;
}

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta);	
	float sum=0;

	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += max_delta[i];
	}
	return sum/lookback ;
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


int findHowManyUpCandleWithPocAtHigh(SCStudyInterfaceRef sc , int idx , int num )
{
	
	int cnt=0;
	for(int i = idx-1 ; i >= idx-num ; i--)
	{
		if(checkValidBar(sc , i ,CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE ) ) cnt++;
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

int countPositiveTable(SCStudyInterfaceRef sc , int idx , int lookback )
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
	
	// Get percent delta
	SCFloatArray percent_delta;
	sc.GetStudyArrayUsingID(3, 10, percent_delta);
	
	int count_of_positive = 0;
	for(int k = idx ; k > idx-lookback ; k--)   //endIndex
	{
		if(delta[k] > 0 ) count_of_positive++ ;
		if(delta_chg[k] > 0 ) count_of_positive++ ;
		if(max_delta[k] > 0 ) count_of_positive++ ;
		if(min_delta[k] > 0 ) count_of_positive++ ;
		if(percent_delta[k] > 0 ) count_of_positive++ ;
	}
	return count_of_positive ;
	
}



int findBreakHighOfPrevBar(SCStudyInterfaceRef sc ,int index , int lookback  )
{
	int count = 0;
	int endIndex = index-lookback;
	
	if(endIndex < 0) return 1000;
	
	for(int i = index ; i >= endIndex ; i--)
	{
		if(sc.High[i] > sc.High[i-1]) count++ ;
	}
	return count;
	
}


int numOfBarThatNot5level(SCStudyInterfaceRef sc ,int index , int lookback  )
{
	int count = 0;
	int endIndex = index-lookback;
	
	
	if(endIndex < 0) return 1000;
	
	for(int i = index ; i >= endIndex ; i--)
	{
		int numPriceLevel = sc.VolumeAtPriceForBars->GetSizeAtBarIndex(i) ;	
		
		if(numPriceLevel != 5) count++ ;
	}
	return count;
	
}

int numTouchHighOfEndIndex(SCStudyInterfaceRef sc ,int index , int lookback  )
{
	
	int count = 0;
	int endIndex = index-lookback;
	
	
	if(endIndex < 0) return 1000;
	
	for(int i = index-1 ; i >= endIndex ; i--)
	{
		if(sc.High[i] >= sc.High[index]) count++ ;
		
	}
	return count;
	
	
	
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











