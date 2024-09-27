














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


#define MAX_VECTOR_SIZE 100000


#define WIN 1 
#define LOSS 2
#define INPROGRESS 3

#define TP_ON_TICKS 40
#define SL_ON_TICKS 5

int uniqueNumber = 4245 ;


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

struct st_SimTrade {
    int tradeNumber;
	int indexOfPattern ;
	int result ;
   
	st_SimTrade(int tnb , int iop , int rs) : tradeNumber(tnb) , indexOfPattern(iop) , result(rs)  {}
};



int isThisStartBarOFGoodSellOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodSellOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodSellOFStartBar>& detectedGoodSellStartBars , 
						  std::vector<st_GoodSellOF>& detectGoodSellOF);						  

//std::vector<st_GoodBuyOFStartBar> detectedGoodBuyStartBars;



float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar , int slIndex  ) ;

void simTrade(SCStudyInterfaceRef sc , std::vector<st_GoodSellOF>& detectGoodSellOF , std::vector<st_SimTrade>& detectTrade) ;

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


SCDLLName("SUA FIND GOOD SELL LONG RENGE")



SCSFExport scsf_FindGoodSellLongRenge(SCStudyInterfaceRef sc)
{
	
	
	
	SCString msg;
	SCInputRef i_minVol = sc.Input[0];
	SCInputRef i_m5_chart_number = sc.Input[1];
	SCInputRef i_draw_to_chart_3 = sc.Input[2];
	
	// Section 1 - Set the configuration variables and defaults
	
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA FIND GOOD Sell Long Range";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 

		sc.GraphRegion = 0;

		sc.Subgraph[0].Name = "Good sell start bar";
		sc.Subgraph[0].DrawStyle = DRAWSTYLE_LINE;
		sc.Subgraph[0].PrimaryColor = RGB (0, 255, 0);

		i_minVol.Name = "Min Volime Of good sell orderflow start bar";
		i_minVol.SetInt(0);

		// Add an input for the M5 chart number

        i_m5_chart_number.Name = "M5 Chart Number";
        i_m5_chart_number.SetInt(7);  // Default to chart number 7
		
		i_draw_to_chart_3.Name = "Draw To Chart 3 or not";
        i_draw_to_chart_3.SetYesNo(false);  // Default to chart number 7
		

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

	



	auto detectedGoodSellStartBars = static_cast<std::vector<st_GoodSellOFStartBar>*>(sc.GetPersistentPointer(1));

	if (!detectedGoodSellStartBars) {
        detectedGoodSellStartBars = new std::vector<st_GoodSellOFStartBar>();
        sc.SetPersistentPointer(1, detectedGoodSellStartBars);
    }

	auto detectGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(2));

	if (!detectGoodSellOF) {
        detectGoodSellOF = new std::vector<st_GoodSellOF>();
        sc.SetPersistentPointer(2, detectGoodSellOF);
    }

	auto sim1s = static_cast<std::vector<st_SimTrade>*>(sc.GetPersistentPointer(3));

	if (!sim1s) {
        sim1s = new std::vector<st_SimTrade>();
        sc.SetPersistentPointer(3, sim1s);
    }
	
	auto fillterGoodSellOF = static_cast<std::vector<st_GoodSellOF>*>(sc.GetPersistentPointer(4));

	if (!fillterGoodSellOF) 
	{
		fillterGoodSellOF = new std::vector<st_GoodSellOF>();
		sc.SetPersistentPointer(4, fillterGoodSellOF);
	}		
	


	// Get the M5 chart number from input
    int M5ChartNumber = i_m5_chart_number.GetInt();
	
	int minVol = i_minVol.GetInt();
	
	bool drawToChart3 = i_draw_to_chart_3.GetYesNo() ;
	
	
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

	if (sim1s != NULL)
		sim1s->clear() ;
	
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
	
	
	
	// 3. draw rectangle of good buy OF
	for (int i = 0; i < detectGoodSellOF->size(); i++)
	{
		//float howMuchMove = findMaxGoDownBeforeBreakHigh(sc,detectGoodSellOF->at(i).startBarIndex , detectGoodSellOF->at(i).endBarIndex) ;
	    //int hmm = sc.PriceValueToTicks(howMuchMove) ;
	
		//if(hmm < 40)
			//continue;
		
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
		
		
		// draw again to chart 3
		if(drawToChart3)
		{			
			rectangle.Clear();
			rectangle.ChartNumber = 3;
			rectangle.DrawingType = DRAWING_RECTANGLEHIGHLIGHT;
			rectangle.AddAsUserDrawnDrawing = 1;
			rectangle.BeginIndex = detectGoodSellOF->at(i).startBarIndex;
			rectangle.EndIndex = detectGoodSellOF->at(i).endBarIndex;
			rectangle.BeginValue = sc.High[detectGoodSellOF->at(i).startBarIndex];
			rectangle.EndValue = sc.Low[detectGoodSellOF->at(i).endBarIndex];
			rectangle.Color = RGB(255, 0, 0);
			rectangle.SecondaryColor = RGB(255, 165, 0);  // Orange color
			rectangle.LineWidth = 1;
			rectangle.TransparencyLevel = 80;
			rectangle.AddMethod = UTAM_ADD_OR_ADJUST;
			rectangle.LineNumber = detectGoodSellOF->at(i).lineNumber ;
			rectangle.AllowCopyToOtherCharts = true;
			sc.UseTool(rectangle);
			
			
		}
		
	}
	
	
	if(sc.UpdateStartIndex == 0)
	{
		SCString filePath = sc.DataFilesFolder() + "CollectedDataGoodSellLongRange.txt";   
		

		// Open the file in append mode
		std::ofstream outputFile;
		if (sc.Index == 0)
		{
			// If it's the first bar, overwrite the file
			outputFile.open(filePath.GetChars(), std::ios::out);
		}
		else
		{
			// Otherwise, append to the file
			outputFile.open(filePath.GetChars(), std::ios::app);
		}

		// Check if the file opened successfully
		if (!outputFile.is_open())
		{
			SCString error;
			error.Format("Failed to open file: %s", filePath.GetChars());
			sc.AddMessageToLog(error, 1);
			return;
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
			//float howMuchMove = findMaxGoDownBeforeBreakHigh(sc,detectGoodSellOF->at(i).startBarIndex , detectGoodSellOF->at(i).endBarIndex) ;
			//int hmm = sc.PriceValueToTicks(howMuchMove) ;

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
		
			int num_up_candle_poc_at_high = findHowManyUpCandleWithPocAtHigh( sc , endIdx , 9 );   //  9
			
			int count_of_positive = 0;
			for(int k = endIdx ; k > endIdx-7 ; k--)   //endIndex
			{
				if(delta[k] > 0 ) count_of_positive++ ;
				if(delta_chg[k] > 0 ) count_of_positive++ ;
				if(max_delta[k] > 0 ) count_of_positive++ ;
				if(min_delta[k] > 0 ) count_of_positive++ ;
				if(percent_delta[k] > 0 ) count_of_positive++ ;
			}
			
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
			
		    //if(hmm < 40) 	continue;			
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
			//if(count_of_positive_2 >= 2 && count_of_positive_2 <= 4 )  continue; 
			//if(count_of_positive_2 > 7)  continue;			
			
			if(count_of_positive_3 >= 4 && count_of_positive_3 <= 5)  continue;  
			if(count_of_positive_3 >= 12 )  continue; 
			
			if(not5Level >= 1 )  continue; 
			
			//if(div_cum_vol < 0.61)  continue;
			if(div_cum_vol > 2)  continue;
			
			if(div_vol_st_ed_idx < 0.18)  continue;  
			if(div_vol_st_ed_idx > 12)  continue; 
            if(div_vol_ed_prev_idx < 0.5 && vol[endIdx] > 18)  continue;   			
			if(num_touch_vwap_last_8 > 1)  continue; 
			if(num_touch_vwap_last_50 > 15)  continue;
			if(div_mean_min_del > 6)  continue;  // //5
			if(vol[endIdx-1] < 80)  continue;
			
			if(brkHigh >= 30 )    continue;
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < 1000 ) continue;				
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < mean5/2 ) continue;	
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < mean5*1 ) continue;	
			//if(meanDelta5*3 >  abs( delta[detectGoodSellOF->at(i).endBarIndex] )  ) continue;
			//if(meanMinDelta10 > -30 ) continue;			
			//if(meanMaxDelta10 > 100 ) //continue;					
			//if(sc.PriceValueToTicks(atrValue) > 1)  continue;			
			if(vol_per_sec[endIdx] < 0.52 )  continue;			//1.4
			//if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex >= 3 && vol[detectGoodSellOF->at(i).endBarIndex] <= 1000) //continue;			
			//if(sc.PriceValueToTicks( vwap[detectGoodSellOF->at(i).endBarIndex]-sc.Close[detectGoodSellOF->at(i).endBarIndex] ) <= -3)  continue;			
			if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= 9)  continue;
			if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= -9 && sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) <= 8)  continue;
			//if(cum_vol_day[detectGoodSellOF->at(i).endBarIndex] > 20000)  continue;
			
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
			
			
		
			// Write the data to the file
			outputFile << "Trade: " << i+1 ;
			outputFile << ", si: " << detectGoodSellOF->at(i).startBarIndex ;
			outputFile << " , ei: " << detectGoodSellOF->at(i).endBarIndex ;
			outputFile << " , diff idx: " << detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex ;
			//outputFile << " , vwap: " << sc.PriceValueToTicks( vwap[detectGoodBuyOF->at(i).endBarIndex]-sc.Close[detectGoodBuyOF->at(i).endBarIndex] )  ;
			outputFile << " , vol: " << vol[detectGoodSellOF->at(i).endBarIndex] ;
			outputFile << " , mean 5 : " << mean5  ;
			outputFile << " , vol/sec: " << vol_per_sec[detectGoodSellOF->at(i).endBarIndex] ;
			//outputFile << " , cvd: " << cum_vol_day[detectGoodBuyOF->at(i).endBarIndex] ;meanDelta3
			outputFile << " , mean delta 5: " << meanDelta5;
			outputFile << " , delta: " << delta[detectGoodSellOF->at(i).endBarIndex] ;  
			//outputFile << " , cumdelta1: " << cum_delta_low[detectGoodBuyOF->at(i).startBarIndex] ;
			//outputFile << " , cumdelta2: " << cum_delta_low[detectGoodBuyOF->at(i).endBarIndex] ;
			//outputFile << " , min delta: " << min_delta[detectGoodBuyOF->at(i).endBarIndex] ;
			//outputFile << " , max delta: " << max_delta[detectGoodBuyOF->at(i).endBarIndex] ;
			outputFile << " , atr: " << sc.PriceValueToTicks(atrValue)  ;
			outputFile << " , ema200: " << sc.PriceValueToTicks(sc.Close[detectGoodSellOF->at(i).endBarIndex] - emaValue)  ;
			//outputFile << " , mean min delta 10 : " << meanMinDelta10  ;
			//outputFile << " , mean max delta 10 : " << meanMaxDelta10  ;
			//outputFile << " , max price move: " /*<< std::setprecision(4) */<< hmm ;
			outputFile << " , time : " << date << "\n";
			
		}
		
		//outputFile << " \ntotal trade : " <<  detectGoodBuyOF->size() << "\n" ;
		//outputFile << sc.TickSize;
		outputFile << sc.PriceValueToTicks(sc.High[sc.ArraySize-1] ) ;
		outputFile << "\nSL: 12 for all trade , Trade Direction is Long only , dont look at si ,ei for analsis\n"  ;	
		outputFile << "if max price move less than 12 mean loss"  ;		
		//outputFile << "I want to know which delta,TP,time range to get best profit"  ;	
		outputFile << "\nsim all trade , which tp level to get best profit , for example , if max price move:20 then you check each trade if max price move < 20 mean loss , if max price move >= 20 mean win\n"  ;	
		outputFile << "and sim total profit lost each tp level"  ;
		//outputFile << "can you fillter trade by delta range,vol range,atr range, diff between si and ei range to get better win rate"  ;	
		
		//can you fillter trade by delta , diff between si and ei range to get better win rate


				 
		// Close the file
		outputFile.close();
		
		
		
		
		// ################################## SIM ###############################################
		simTrade( sc , *fillterGoodSellOF , *sim1s) ;
		// ################################## SIM ###############################################
		
		
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

float findMaxGoDownBeforeBreakHigh(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar , int slIndex) 
{
	
	
	int high ;
		
	if(slIndex == indexOfEndtBar)
	{
		high = 	sc.PriceValueToTicks(sc.High[indexOfEndtBar]) + 1 ;// SL at 
	}
	else if(slIndex == indexOfStartBar)
	{
		high = 	sc.PriceValueToTicks(sc.High[indexOfStartBar]) + 1 ;// SL at 	
	}
	
	
	//int high = 	sc.PriceValueToTicks(sc.High[indexOfStartBar]) + 1 ;// SL at 
	//int high = 	sc.PriceValueToTicks(sc.High[indexOfEndtBar]) + 1 ;// SL at 
	
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

void simTrade(SCStudyInterfaceRef sc , std::vector<st_GoodSellOF>& detectGoodSellOF , std::vector<st_SimTrade>& detectTrade)
{
	int tradeNumber = 0 ;
	float tp = TP_ON_TICKS*sc.TickSize;
	
	
	detectTrade.clear() ;
		
	// loop in good Sell OF
	for (int i = 0; i < detectGoodSellOF.size(); i++)
	{
		
		
		
			
			
		int sellIndex = i;
		
		int entryIndex = detectGoodSellOF[i].endBarIndex ;
		float entryPrice = sc.Low[detectGoodSellOF[i].endBarIndex] ;
		float stopLoss = sc.Low[detectGoodSellOF[i].endBarIndex] + SL_ON_TICKS*sc.TickSize ;   //   ------------- good
		//float stopLoss = sc.High[detectGoodSellOF[i].startBarIndex] + sc.TickSize ;
		
		
		int i_vol = 9;	
		float cum_vol = cummulateVol( sc , entryIndex ,i_vol) ;
		float cum_vol_i_10 = cummulateVol( sc , entryIndex-i_vol ,i_vol) ;
		float div_cum_vol = cum_vol/cum_vol_i_10 ;
		
		
		
		if(div_cum_vol < 0.61)    stopLoss = sc.High[detectGoodSellOF[i].startBarIndex] + sc.TickSize ;
		
		tp = entryPrice - TP_ON_TICKS*sc.TickSize  ;
		
		int indexOfPattern ;
		int result ;
		
		// loop from entry bar to end
		for(int j = entryIndex+1 ; j < sc.ArraySize-1; j++)
		{
			// if break high win
			if(sc.Low[j] <= tp)
			{
				// win
				tradeNumber++;
				indexOfPattern = sellIndex;
				result = TP_ON_TICKS ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
			    
				break;				
												
				
			}// if breaK High loss
			else if( sc.High[j] >= stopLoss)
			{
				// loss
				tradeNumber++;
				indexOfPattern = sellIndex;
				result = -1*SL_ON_TICKS ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
				break;		
			}
		}
		
		
	}
	
	
	
	
	SCString filePath = sc.DataFilesFolder() + "Sim Good Sell Long Renge Trades.txt";   		

	// Open the file in append mode
	std::ofstream outputFile;
	if (sc.Index == 0)
	{
		// If it's the first bar, overwrite the file
		outputFile.open(filePath.GetChars(), std::ios::out);
	}
	
	// Check if the file opened successfully
	if (!outputFile.is_open())
	{
		SCString error;
		error.Format("Failed to open file: %s", filePath.GetChars());
		sc.AddMessageToLog(error, 1);
		return;
	}
	
	outputFile << "if div cum vol < 0.61 then use SL at start bar\n\n"  ;



	// it is vol/sec
	SCFloatArray vol_per_sec;
	sc.GetStudyArrayUsingID(3, 35, vol_per_sec);


	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	

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

	// for bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
	
	// for count good buy OF
	SCFloatArray goodBuyArray;
	sc.GetStudyArrayUsingID(9 , 1, goodBuyArray);
	
	// for good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);

	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(13, 0, vwap);	




    int totalProfit = 0 ;
	int numberWin = 0;
	int numberLoss = 0;
	
	for(int i = 0 ; i < detectTrade.size() ; i++)
	{
		int startIndex = detectGoodSellOF[detectTrade[i].indexOfPattern].startBarIndex ;
		int endIndex =  detectGoodSellOF[detectTrade[i].indexOfPattern].endBarIndex ;
		float volOfStartBar = detectGoodSellOF[detectTrade[i].indexOfPattern].volume_of_start_bar ;
		float volOfEndBar = detectGoodSellOF[detectTrade[i].indexOfPattern].volume_of_end_bar ;
		int rs = detectTrade[i].result ;
		
		SCDateTime t1 = sc.BaseDateTimeIn[endIndex];   //detectDBwithGoodBuyOF[i].indexOfGoodOfEndBar
		SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);

		
		
				
		int i_vol = 9;	
		float cum_vol = cummulateVol( sc , endIndex ,i_vol) ;
		float cum_vol_i_10 = cummulateVol( sc , endIndex-i_vol ,i_vol) ;
		float div_cum_vol = cum_vol/cum_vol_i_10 ;
		
		// for mean 3 /
		float mean3 = meanVol( sc , endIndex, 7 ) ; // 3 , 50 , spacial_mean
		
		float vol_per_mean = volOfEndBar/mean3 ;

		float vps = vol_per_sec[endIndex] ;
		
		SCDateTime temp = bar_dur_array[endIndex] ;
		float bar_sec_ed = temp.GetTimeInSeconds() ;
		
		temp = bar_dur_array[startIndex] ;
		float bar_sec_st = temp.GetTimeInSeconds() 		;
		
		int num_up_candle_poc_at_high = findHowManyUpCandleWithPocAtHigh( sc , endIndex , 5 );
			
		int count_of_positive = 0;
		for(int k = endIndex ; k > endIndex-2 ; k--)   //endIndex
		{
			if(delta[k] > 0 ) count_of_positive++ ;
			if(delta_chg[k] > 0 ) count_of_positive++ ;
			if(max_delta[k] > 0 ) count_of_positive++ ;
			if(min_delta[k] > 0 ) count_of_positive++ ;
			if(percent_delta[k] > 0 ) count_of_positive++ ;
		}
		
		int count_of_positive_2 = countPositiveTable( sc , endIndex , 2 )   ;
		int count_of_positive_3 = countPositiveTable( sc , endIndex , 3 )   ;
		int count_of_positive_7 = countPositiveTable( sc , endIndex , 7 )   ;
		int count_of_positive_28 = countPositiveTable( sc , endIndex , 28 )   ;
		
		// count good sell OF
		int num = 10;
		int cnt_good_sell = 0;
		
		for(int i = endIndex-1 ; i >= endIndex-num ; i--)
		{
			if(goodSellArray[i] == 1) 
			{
				cnt_good_sell++;
				break ;
			}
		}
		
		// count good buy OF
		num = 5;
		int cnt_good_buy = 0;		 
		for(int i = endIndex-1 ; i >= endIndex-num ; i--)    // startIndex
		{
			if(goodBuyArray[i] == 1) cnt_good_buy++;
		}
		
		//float volOfStartBar = vol[startIndex] ;
		//float volOfEndBar = vol[endIndex] ;
		float div_vol_st_ed_idx = volOfStartBar/volOfEndBar ;
			
		float div_vol_ed_prev_idx = vol[endIndex]/vol[endIndex-1] ;	
			
		int num_touch_vwap = findHowManyTouchVwap( sc , endIndex , 50 );   //30
		
		int i_5 = 3;
		float meanMinDel = meanMinDelta( sc , endIndex, i_5 ) ;
		float meanMinDelta5_i_5 = meanMinDelta( sc , endIndex-i_5 ,i_5 ) ;
		float div_mean_min_del = meanMinDel/meanMinDelta5_i_5 ;
		
		int brkHigh = findBreakHighOfPrevBar( sc ,endIndex, 40 );
		
		int not5Level = numOfBarThatNot5level( sc ,endIndex, 5 )  ;
		
		int numTouchEdIdx =  numTouchHighOfEndIndex( sc ,endIndex, 10 )  ;
		
		
		
		float hmm ;
		
		if(div_cum_vol < 0.61)  
		{
			hmm = findMaxGoDownBeforeBreakHigh( sc , startIndex ,endIndex , startIndex );   //  ---------- good
		}
		else
		{
			hmm = findMaxGoDownBeforeBreakHigh( sc , startIndex ,endIndex , endIndex );   //  ---------- good
		}
		
		
		//float hmm = findMaxGoDownBeforeBreakHigh( sc , startIndex ,startIndex  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		
		
		
		
		outputFile << "Trade: " << detectTrade[i].tradeNumber ;
		outputFile << ", st ix : " << startIndex ;
		outputFile << ", ed ix : " << endIndex ;
		outputFile << ", dff ix : " << endIndex-startIndex ;
		//outputFile << ", vol st : " << volOfStartBar ;  
		//outputFile << ", vol prv ed : " << vol[endIndex-1]  ; 
		//outputFile << ", vol ed : " << volOfEndBar ;  
		outputFile << ", vol ed/prv : " << div_vol_ed_prev_idx ; 
		outputFile << ", vol st/ed : " << div_vol_st_ed_idx ;  
		outputFile << ", hmn touch vwap : " << num_touch_vwap ;  
		outputFile << ", div_mean_min_del : " << div_mean_min_del ;    
		outputFile << ", vps : " << vol_per_sec[endIndex] ;      
		outputFile << ", barsec st : " << bar_sec_st ;
		outputFile << ", barsec ed : " << bar_sec_ed ;
		outputFile << " , vwap: " << sc.PriceValueToTicks( vwap[endIndex]-sc.Close[endIndex] )  ;
		outputFile << " , div cum vol: " << div_cum_vol  ;
		outputFile << " , num poc high: " << num_up_candle_poc_at_high  ;
		outputFile << " , cnt pos2: " << count_of_positive_2  ;
		outputFile << " , cnt pos3: " << count_of_positive_3  ;
		outputFile << " , cnt pos28: " << count_of_positive_28  ;
		//outputFile << " , cnt gb: " << cnt_good_buy  ;
		//outputFile << " , cnt gs: " << cnt_good_sell  ;
		outputFile << " , cnt brk high: " << brkHigh  ;
		outputFile << " , not 5 lev: " << not5Level  ;
		outputFile << " , num touch ed ix: " << numTouchEdIdx  ;
		outputFile << " , rs: " << rs ;
		outputFile << " , max move : " << howMuchMove << "T" ;
		outputFile << " , date: " << date ;
		outputFile << " \n"  ;
		
		totalProfit += rs;
		
		if(rs > 0)
		{
			numberWin++;
		}
		else
		{
			numberLoss++;
		}
		
	}
	
	outputFile << detectTrade.size() << " \n"  ;
	//outputFile << "Tp: " << 1 + 40*sc.TickSize ; 
	//outputFile << " , Sl: " << 1.2 - 5*sc.TickSize ;
	outputFile << "                           number Win: " << numberWin << "\n" ;
	outputFile << "                           number Loss: " << numberLoss << "\n" ;
	outputFile << "                           Total profit: " << totalProfit << "\n" ;
	
    
	// Close the file
	outputFile.close();	
	
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

















