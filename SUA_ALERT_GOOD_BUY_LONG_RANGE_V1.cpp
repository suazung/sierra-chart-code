









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

 
#define MAX_VECTOR_SIZE 100



#define TP_ON_TICKS 40
#define SL_ON_TICKS 5

int uniqueNumber = 44444 ;

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




int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodBuyOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , 
						  std::vector<st_GoodBuyOF>& detectGoodBuyOF);						  

//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;

float findMaxGoUpBeforeBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) ;

int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num );

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );

float meanMinDelta( SCStudyInterfaceRef sc , int index , int lookback );

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback ) ;

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback) ;

int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum );

int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num )  ;

void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString)  ;

bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )   ;


SCDLLName("SUA ALERT GOOD BUY Long Range")


//This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
SCSFExport scsf_AlertGoodBuyLongRange(SCStudyInterfaceRef sc)
{
	
	
	SCString msg;
	
	SCInputRef i_minBar = sc.Input[0];
	SCInputRef i_tp_in_ticks = sc.Input[1];
	SCInputRef AlertSound = sc.Input[2];
	
	
	int BUY_SIGNAL = 1 ;
	int M1ChartNumber = 7 ;	
	int GOOD_BUY_LONG_RENGE_OF_CODE = 3000   ;
	;
	SCString m1TimeString , currentTimeString;
	
	int persistentUniqueValue = sc.GetPersistentInt(5);   // 5 is key
	
	int lastProcessedBarIndex = sc.GetPersistentInt(6);
	//static int lastProcessedBarIndex = -1;
	
	// Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		
		sc.GraphName = "SUA ALERT GOOD BUY Long Range";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 2;
		
		sc.Subgraph[0].Name = "BUY SIGNAL";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_STAR;
        sc.Subgraph[0].LineWidth = 7;
		sc.Subgraph[0].PrimaryColor = RGB(255,255,0);
		
		sc.Subgraph[1].Name = "START IDX";
		sc.Subgraph[1].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[1].LineWidth = 7;
		
		sc.Subgraph[2].Name = "END IDX";
		sc.Subgraph[2].DrawStyle = DRAWSTYLE_HIDDEN;
        sc.Subgraph[2].LineWidth = 7;
		
	
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(30);
		
		i_minBar.Name = "min number of bar";
		i_minBar.SetInt(4);
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(2);  // Default alert sound number
		
		return;
	}
	
	
	// Section 2 - Do data processing here
	
	int MIN_START_INDEX = 0;
	int MaxBarLookback = 0;
	
		
	// See if we are capping max bars to check back
	if (MaxBarLookback == 0)
		sc.DataStartIndex = MIN_START_INDEX; // Need at least three bars to calculate
	else
		sc.DataStartIndex = sc.ArraySize - 1 - MaxBarLookback + MIN_START_INDEX;
	
	

	int minBar = i_minBar.GetInt();
	int tp_in_ticks = i_tp_in_ticks.GetInt();

	
	
	auto detectedGoodBuyStartBars = static_cast<std::vector<st_GoodBuyOFStartBar>*>(sc.GetPersistentPointer(1));
	
	if (!detectedGoodBuyStartBars) {
        detectedGoodBuyStartBars = new std::vector<st_GoodBuyOFStartBar>();
        sc.SetPersistentPointer(1, detectedGoodBuyStartBars);
    }
	
	auto detectGoodBuyOF = static_cast<std::vector<st_GoodBuyOF>*>(sc.GetPersistentPointer(2));
	
	if (!detectGoodBuyOF) {
        detectGoodBuyOF = new std::vector<st_GoodBuyOF>();
        sc.SetPersistentPointer(2, detectGoodBuyOF);
    }
	
	// ################## create object st_GoodBuyOF for collect fittered good buy OF
	auto fillterGoodBuyOF = static_cast<std::vector<st_GoodBuyOF>*>(sc.GetPersistentPointer(4));	
	if (!fillterGoodBuyOF) 
	{
		fillterGoodBuyOF = new std::vector<st_GoodBuyOF>();
		sc.SetPersistentPointer(4, fillterGoodBuyOF);
	}		
		
	
		
 
	//A study will be fully calculated/recalculated when it is added to a chart, any time its Input settings are changed,
	// another study is added or removed from a chart, when the Study Window is closed with OK or the settings are applied.
	// Or under other conditions which can cause a full recalculation.
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		for (int i = 0; i < detectGoodBuyOF->size(); i++)
		{
			sc.DeleteUserDrawnACSDrawing(sc.ChartNumber, detectGoodBuyOF->at(i).lineNumber);
		}
		
		// Drawings removed, now clear to avoid re-drawing them again
		detectGoodBuyOF->clear();


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
	if (detectedGoodBuyStartBars != NULL)
		detectedGoodBuyStartBars->clear();
	
	
	if (detectGoodBuyOF != NULL)
		detectGoodBuyOF->clear();
	
	if (fillterGoodBuyOF != NULL)
			fillterGoodBuyOF->clear() ;
	
	
	 // 1.Loop through bars to detect good buy start bar pattern
    for ( int i = sc.DataStartIndex ; i < sc.ArraySize-1; ++i) 
	{
		
		// Check if this is a new bar
		/*if (sc.GetBarHasClosedStatus(i) == BHCS_BAR_HAS_NOT_CLOSED) {
            continue;
        }*/
		
		
		
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
	
	
	
	// Get the study ID3 and SG1 (=0)    | it is delta
	SCFloatArray delta;
	sc.GetStudyArrayUsingID(3, 0, delta);
	
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
	
	// it is min_delta
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);
	
	// it is min_delta
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta);
	
	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(12, 0, vwap);
	
	// for bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
			
	
	
	for (int i = 0; i < detectGoodBuyOF->size(); i++)
	{
		
		int startIdx = detectGoodBuyOF->at(i).startBarIndex ;
		int endIdx = detectGoodBuyOF->at(i).endBarIndex ;
		
		
		SCString date = sc.DateTimeToString( detectGoodBuyOF->at(i).time_of_end_bar, FLAG_DT_COMPLETE_DATETIME);
				
			
		// for mean vol
		float mean3 = meanVol( sc , detectGoodBuyOF->at(i).endBarIndex, 7 ) ; //5
		
		float mean3_i_3 = meanVol( sc , detectGoodBuyOF->at(i).endBarIndex-7, 7 ) ; //5
		
		float div_mean = mean3 / mean3_i_3 ;
		
		// for mean delata
		float meanDelta5 = meanDelta(  sc , detectGoodBuyOF->at(i).endBarIndex-1, 5 ) ;    //5,10
		
		// for mean min delta
		float meanMinDelta10 = meanMinDelta( sc , detectGoodBuyOF->at(i).endBarIndex-1, 10  );
		
		// for mean max delta
		float meanMaxDelta10 = meanMaxDelta( sc , detectGoodBuyOF->at(i).endBarIndex-1, 10 ) ;
		
		SCDateTime temp = bar_dur_array[detectGoodBuyOF->at(i).endBarIndex] ;
		float bar_sec_ed = temp.GetTimeInSeconds() ;
		
		temp = bar_dur_array[detectGoodBuyOF->at(i).startBarIndex] ;
		float bar_sec_st = temp.GetTimeInSeconds() ;
	
	
		float volOfStartBar = vol[detectGoodBuyOF->at(i).startBarIndex] ;
		float volOfEndBar = vol[detectGoodBuyOF->at(i).endBarIndex] ;
		float div_vol_st_ed_idx = volOfStartBar/volOfEndBar ;
		
		int hmnPocAtLow = findHowManyDownCandleWithPocAtLow(sc ,detectGoodBuyOF->at(i).endBarIndex , 9 ) ;
		
		float vol_per_mean = volOfEndBar/mean3 ;
		
		int countPos7 = count_pos_table( sc , endIdx , 7 ) ;	
		int countPos28 = count_pos_table( sc , endIdx , 28 ) ;	//20  25  24
	
		int num_touch_vwap_last_8 = findHowManyTouchVwap( sc , endIdx , 8 );   //30  , 8			
		int num_touch_vwap_last_50 = findHowManyTouchVwap( sc , endIdx , 50 );   //30  , 8
	
	
		// ################################################ Start filter Good Buy OF Here  ############################################
		
		//if(hmm < 40) 	continue;			
		if(detectGoodBuyOF->at(i).time_of_end_bar.GetHour() > 18) continue;	
		if(detectGoodBuyOF->at(i).endBarIndex - detectGoodBuyOF->at(i).startBarIndex < 20 ) continue;		//80    // good
		if(detectGoodBuyOF->at(i).endBarIndex - detectGoodBuyOF->at(i).startBarIndex > 450 ) continue;  //450    // good
		if(bar_sec_ed > 350)  continue;   //               ------------------ good
		if(bar_sec_ed < 3)  continue;             //    ------------------ good
		if(bar_sec_st > 600)  continue; 
		if(div_mean < 0.48)  continue; 
		if(div_mean > 1.12)  continue; 
		if(div_vol_st_ed_idx > 7.5)  continue; 
		if(hmnPocAtLow >= 4)  continue; 		
		if(countPos7 <= 9)     continue; 
		if(countPos7 >= 17 && countPos7 <= 21)     continue; 
		if(countPos7 >= 26)     continue; 		
		if(num_touch_vwap_last_8 >= 5)    continue; 
		if(num_touch_vwap_last_50 >= 16)    continue; 
		if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= 40)  continue;	
	
	
		
		int idxOfGoodBuyOF = i ;
		bool haveStartBarIsSameAsBefore = false;
		float lowOfThisEndBar = sc.Low[endIdx] ;
		float volOfThisEndBar = detectGoodBuyOF->at(i).volume_of_end_bar ;
		int sumOFSameStartIndex = 0;
		for(int j = idxOfGoodBuyOF-1 ; j >= 0; j-- )   // loop in good buy OF from current to 0
		{
			
			// if find start bar index same as this index before then dont want
			if( (startIdx == detectGoodBuyOF->at(j).startBarIndex)  )
			{
				//haveStartBarIsSameAsBefore = true;
				//break;
				if( lowOfThisEndBar < sc.Low[detectGoodBuyOF->at(j).endBarIndex]  ) continue;  // if low of this end bar below low of previous end bar then it not same
				if( (volOfThisEndBar < 50*detectGoodBuyOF->at(j).volume_of_end_bar) && ( detectGoodBuyOF->at(j).volume_of_end_bar <= 20) ) continue; // if volOfThisEndBar more 10x of previous same start bar then it not same
				//if( detectGoodBuyOF->at(j).volume_of_end_bar <= 20)  sumOFSameStartIndex++ ;
				
				sumOFSameStartIndex++ ;
			}
		}
		//if(haveStartBarIsSameAsBefore == true)  continue;
		//if(sumOFSameStartIndex > 0 )  continue;
		
		
		int sumOFTouchHighOfStartIndex = 0;
		for(int k = endIdx-1 ; k >= startIdx+1; k--)
		{
			if(sc.Low[k] <= sc.High[startIdx]+sc.TickSize )   // if touch high of start bar index
			{
				sumOFTouchHighOfStartIndex++;
			}
		}
		if(sumOFTouchHighOfStartIndex > 5 )  continue;  //3    // good
		
		// ########################################### End filter Good Buy OF Here  ############################################
		
		
		
		
		// ####################################################################################################################################
		// ################################################ Put Signal To subgraph###############  ############################################
		// ####################################################################################################################################			
		
			
		sc.Subgraph[0][detectGoodBuyOF->at(i).endBarIndex] = 1;	
		sc.Subgraph[1][detectGoodBuyOF->at(i).endBarIndex] = startIdx;	
		sc.Subgraph[2][detectGoodBuyOF->at(i).endBarIndex] = endIdx;	
		
		
		

		// ######################## Add data to fillterGoodBuyOF Object 
		int idx1 = detectGoodBuyOF->at(i).startBarIndex;
		int v1 = detectGoodBuyOF->at(i).volume_of_start_bar;
		int d1 = detectGoodBuyOF->at(i).delta_of_start_bar;
		SCDateTime t1 = detectGoodBuyOF->at(i).time_of_start_bar;			
		int idx2 = detectGoodBuyOF->at(i).endBarIndex ;
		int v2 = detectGoodBuyOF->at(i).volume_of_end_bar;
		int d2 = detectGoodBuyOF->at(i).delta_of_end_bar;
		SCDateTime t2 = detectGoodBuyOF->at(i).time_of_end_bar;	
		int ln = detectGoodBuyOF->at(i).lineNumber ;						
		fillterGoodBuyOF->emplace_back(idx1, v1, d1 , t1 ,idx2 ,v2 , d2 , t2 , ln );
		
			
		
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
			
			int slInTick = 1 + sc.PriceValueToTicks(sc.High[idx_of_end_bar] - sc.Low[idx_of_end_bar]);
			int tpInTick = tp_in_ticks ;
			
			int slIndex = idx_of_end_bar;
			
			persistentUniqueValue++ ;
				
			if(persistentUniqueValue > 500)
				persistentUniqueValue = 0;
				
			sc.SetPersistentInt(5, persistentUniqueValue);	
					
					
			int uniqNumber = persistentUniqueValue + GOOD_BUY_LONG_RENGE_OF_CODE  ;
			message.Format("BUY | index = %d , uniq = %d" , idx_of_end_bar , uniqNumber   ); // sc.CurrentSystemDateTime
			sc.AddMessageToLog(message, 0);
			
			bool CanGetTime = get_M1Time_currentTime_String( sc ,M1ChartNumber , slIndex ,  BUY_SIGNAL , m1TimeString , currentTimeString ) ;
					
					
			if ( /*idx_of_end_bar == sc.ArraySize - 1 - 1  &&*/ CanGetTime /*&& sc.GetBarHasClosedStatus(idx_of_end_bar)==BHCS_BAR_HAS_CLOSED*/) 
			{
				if (sc.IsFullRecalculation) return;
				
				sendHttpPostMessage( sc, uniqNumber ,  BUY_SIGNAL ,  m1TimeString , currentTimeString) ;
				 // Update the last bar index where the message was sent
				//sc.SetPersistentInt(6, currentIndex - 1);
				
			}				
			
			textToTelegram.Format("GU BUY NOW! at %.4f                                        ", sc.High[idx_of_end_bar] );
			textToTelegram += "\n";
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("SL at %.4f [%dT]                                        " , sc.Low[idx_of_end_bar] - sc.TickSize, slInTick );
			textToTelegram.Append("\n");
			textToTelegram.AppendFormat("TP at %.4f [%dT]                                        " , sc.High[idx_of_end_bar] + (tp_in_ticks*sc.TickSize), tpInTick );
			
			 // Trigger the alert
			sc.SetAlert(AlertSound.GetAlertSoundNumber()-1 , textToTelegram );
			
		}
		
				
	}
	
	
	
	// 3. draw rectangle of good buy OF
	for (int i = 0; i < detectGoodBuyOF->size(); i++)
	{
		float howMuchMove = findMaxGoUpBeforeBreakLow(sc,detectGoodBuyOF->at(i).startBarIndex , detectGoodBuyOF->at(i).endBarIndex) ;
	    int hmm = sc.PriceValueToTicks(howMuchMove) ;
	
		//if(hmm < 40)
			//continue;
		
		// **************************** Draw GOOD BUY OF HERE *********************************************
		s_UseTool rectangle;
		rectangle.Clear();
		rectangle.ChartNumber = sc.ChartNumber;
		rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
		rectangle.AddAsUserDrawnDrawing = 1;
		rectangle.BeginIndex = detectGoodBuyOF->at(i).startBarIndex;
		rectangle.EndIndex = detectGoodBuyOF->at(i).endBarIndex;
		rectangle.BeginValue = sc.High[detectGoodBuyOF->at(i).endBarIndex];
		rectangle.EndValue = sc.Low[detectGoodBuyOF->at(i).startBarIndex];
		rectangle.Color = RGB(255, 0, 0);
		rectangle.SecondaryColor = RGB(255, 255, 0);  // Yellow color
		rectangle.LineWidth = 1;
		rectangle.TransparencyLevel = 75;
		rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
		rectangle.LineNumber = detectGoodBuyOF->at(i).lineNumber ;
		rectangle.AllowCopyToOtherCharts = true;
		sc.UseTool(rectangle);
						
		SCString msg;
		msg.Format("start bar : %d | end bar : %d \n" , detectGoodBuyOF->at(i).startBarIndex , detectGoodBuyOF->at(i).endBarIndex );
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
	int sl = 	sc.PriceValueToTicks(sc.Low[indexOfEndtBar]) - 1 ;// SL at 
	
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


int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex)
{
	
	
	return -1 ;
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


int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num )
{
	
	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(12, 0, vwap);	
		
		
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









