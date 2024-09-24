






// -------- try vwap 1 min filter by 20

#include "sierrachart.h"

#define CHECK_5LEVEL 1 
#define CHECK_5LEVEL_AND_POCATBOTTOM 2 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_UPCANDLE 3
#define CHECK_5LEVEL_AND_POCATTOP_AND_UPCANDLE 4  

#define CHECK_5LEVEL_AND_POCATTOP 5 
#define CHECK_5LEVEL_AND_POCATBOTTOM_AND_DOWNCANDLE 6

#define MAX_VECTOR_SIZE 15000

#define TP_1 30
#define BE_1 1
#define TP_2 50
#define BE_2 25
#define TP_3 100
#define BE_3 50


 
SCDLLName("SUA FIND DB GOOD OF")

int uniqueNumber = 34342 ;
int uniqueNumberForPivot = 861 ;


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


struct st_SimTrade {
    int tradeNumber;
	int indexOfPattern ;
	int result ;
   
	st_SimTrade(int tnb , int iop , int rs) : tradeNumber(tnb) , indexOfPattern(iop) , result(rs)  {}
};


bool iSPivotLow(SCStudyInterfaceRef sc , int index, int pivotLength) ;

int isThisStartBarOFGoodBuyOrderflow(SCStudyInterfaceRef sc , int startIndex);

bool checkValidBar(SCStudyInterfaceRef sc , int idx , int condition);

void findGoodBuyOrderFlow(SCStudyInterfaceRef sc, 
                          std::vector<st_GoodBuyOFStartBar>& detectedGoodBuyStartBars , 
						  std::vector<st_GoodBuyOF>& detectGoodBuyOF);
						  
float findMaxGoUpBeforeBreakLow(SCStudyInterfaceRef sc ,int indexOfStartBar , int indexOfEndtBar) ;		

void simTrade(SCStudyInterfaceRef sc , std::vector<st_DoubleBottomWithGoodOF>& detectDBwithGoodBuyOF , std::vector<st_SimTrade>& detectTrade , int tp_in_ticks) ;

float meanVol( SCStudyInterfaceRef sc , int index , int lookback );

float meanDelta( SCStudyInterfaceRef sc , int index , int lookback );

int cummulateVol(SCStudyInterfaceRef sc , int index , int lookback) ;

int findHowManyTouchLow(SCStudyInterfaceRef sc , int stIndex , int edIndex);

int findHowManyDownCandleWithPocAtLow(SCStudyInterfaceRef sc , int idx , int num );

int cummulateDelta(SCStudyInterfaceRef sc , int index , int lookback) ;

int findBreakLowOfPrevBar(SCStudyInterfaceRef sc ,int index , int lookback  ) ;

int findHowManyTouchVwap(SCStudyInterfaceRef sc , int idx , int num )  ;

int count_pos_table(SCStudyInterfaceRef sc , int idx , int lookbackColum );

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback );

int cummulateMinDelta(SCStudyInterfaceRef sc , int index , int lookback);

int cummulateMaxDelta(SCStudyInterfaceRef sc , int index , int lookback) ;


//This is the basic framework of a study function. Change the name 'TemplateFunction' to what you require.
SCSFExport scsf_FindDb(SCStudyInterfaceRef sc)
{
	
	SCString msg;
	
	SCInputRef i_minBar = sc.Input[0];
	SCInputRef i_pivotLength = sc.Input[1];
	SCInputRef i_tp_in_ticks = sc.Input[2];
	SCInputRef i_special_mean = sc.Input[3];
	SCInputRef i_vol_per_sec = sc.Input[4];
	SCInputRef AlertSound = sc.Input[5];
	
	
	// Section 1 - Set the configuration variables and defaults
	if (sc.SetDefaults)
	{
		sc.GraphName = "SUA FIND DB WITH GOOD OF";

		sc.AutoLoop = 0;  //Automatic looping is disabled. 
		
		sc.GraphRegion = 0;
		
		i_pivotLength.Name = "number of bar each side | ex. 4 is left 4 bars , right 4 bars";
		i_pivotLength.SetInt(2);
		
		i_tp_in_ticks.Name = "TP in ticks";
		i_tp_in_ticks.SetInt(100);
		
		i_minBar.Name = "min number of bar";
		i_minBar.SetInt(4);
		
		i_special_mean.Name = "spacial mean";
		i_special_mean.SetInt(3);
		
		i_vol_per_sec.Name = "max volume per sec";
		i_vol_per_sec.SetFloat(3.6);
		
		AlertSound.Name = "Alert Sound";
        AlertSound.SetAlertSoundNumber(3);  // Default alert sound number
		
		return;
	}
	
	
	// Section 2 - Do data processing here
	
	// Section 2 - Do data processing here
	
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
	auto sim1s = static_cast<std::vector<st_SimTrade>*>(sc.GetPersistentPointer(6));
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
	
	if (!sim1s) {
        sim1s = new std::vector<st_SimTrade>();
        sc.SetPersistentPointer(6, sim1s);
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
	
	if (sim1s != NULL)
		sim1s->clear();
	
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
	
	
	 // 2. find end bar of good sell OF
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
	
	
	// 4. Alert , draw 3T zone , draw good buy OF
	for (int i = 0; i < detectDBWithGoodOF->size(); i++)   
	{
		
		float hmm = findMaxGoUpBeforeBreakLow( sc , detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDBWithGoodOF->at(i).indexOfGoodOfEndBar  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		if(howMuchMove < tp_in_ticks) 	continue;
		
		
		
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
		
		// draw good sell OF
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
	
	
	// ########################           5. write to file                  ################################################
	if(sc.UpdateStartIndex == 0)
	{
		SCString filePath = sc.DataFilesFolder() + "find_DB_with_good_OF.txt";   		

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
		
		
		
		outputFile << "detect Double bottom with good OF  : " << "\n" ;
		
		for (int i = 0; i < detectDBWithGoodOF->size(); i++)
		{
			
			
			
			
			float hmm = findMaxGoUpBeforeBreakLow( sc , detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDBWithGoodOF->at(i).indexOfGoodOfEndBar  );
			int howMuchMove = sc.PriceValueToTicks(hmm) ;
			
						
			SCFloatArray vol_per_sec;
			sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
			float vpsOfEndBar = vol_per_sec[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] ;					
			
			int diff_idx_pivot_goodOF = detectDBWithGoodOF->at(i).indexOfGoodOfStartBar - detectDBWithGoodOF->at(i).indexOfPivotLow ;
			int diff_index_st_ed_GF_bar = detectDBWithGoodOF->at(i).indexOfGoodOfEndBar - detectDBWithGoodOF->at(i).indexOfGoodOfStartBar  ;
			
			SCDateTime t = sc.BaseDateTimeIn[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar];
			SCString date = sc.DateTimeToString( t, FLAG_DT_COMPLETE_DATETIME);
			
			// for mean 3 last 3 vol , if idx = 10 , i = 10,9,8
			float mean3 = meanVol( sc , detectDBWithGoodOF->at(i).indexOfGoodOfEndBar, spacial_mean ) ; 			//5
			// for mean 3 last 3 vol , if idx = 10 , i = 7,6,5
			float mean3_last_3 = meanVol( sc , detectDBWithGoodOF->at(i).indexOfGoodOfEndBar-spacial_mean , spacial_mean ) ; //5,5
		
			outputFile << "i : " << i ; 
			outputFile << " | pv idx : " << detectDBWithGoodOF->at(i).indexOfPivotLow  ; 
			outputFile << " | gf idx : " <<  detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ;
			outputFile << " | diff idx PV GF : " << diff_idx_pivot_goodOF ;  
			outputFile << " | diff idx st ed GF : " << diff_index_st_ed_GF_bar ;  
			outputFile << " | VPS : " << vpsOfEndBar ;	
			//outputFile << " | ATR : " <<  ;			
			outputFile << " | mean 3 : " << mean3 ;
			outputFile << " | mean i-3 : " << mean3_last_3 ;
			outputFile << " | Move : " << howMuchMove << "T" ;
			outputFile << " | date : " << date ;	
			outputFile << "\n";
		}		
		
		// Close the file
		outputFile.close();
		
		
		
//      ################################################                        ############################################
//      ################################################ Get pattern and filter ############################################
//      ################################################                        ############################################


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
		
		
		
					
		for (int i = 0; i < detectDBWithGoodOF->size(); i++)
		{
			// for get index on m5 chart
			int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, detectDBWithGoodOF->at(i).indexOfGoodOfEndBar);   // (chart number of m5 , sc.Index)
			
			SCDateTime t1 = sc.BaseDateTimeIn[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar];
			SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
			
			float hmm = findMaxGoUpBeforeBreakLow( sc , detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ,detectDBWithGoodOF->at(i).indexOfGoodOfEndBar  );
			int howMuchMove = sc.PriceValueToTicks(hmm) ;
			
			int diff_pv_gf_idx = detectDBWithGoodOF->at(i).indexOfGoodOfStartBar  - detectDBWithGoodOF->at(i).indexOfPivotLow  ;
			
			int diff_st_ed_idx = detectDBWithGoodOF->at(i).indexOfGoodOfEndBar - detectDBWithGoodOF->at(i).indexOfGoodOfStartBar ;

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
		
		
			
			// for count good Sell OF
			SCFloatArray goodSellArray;
			sc.GetStudyArrayUsingID(9 , 0, goodSellArray);
			
			// for good Buy OF
			SCFloatArray goodBuyArray;
			sc.GetStudyArrayUsingID(9 , 1, goodBuyArray);
			
			
			
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
			
			
			int countPos7 = count_pos_table( sc , endIdx , 7 ) ;	
			int countPos28 = count_pos_table( sc , endIdx , 28 ) ;	//20  25  24
		
			
			if(haveSameStartBar /*|| haveSameEndBar*/ )  continue;
			if(vol[endIdx-1] < 249)   continue;
			//if(haveSameEndBar )  continue;
			//if(diff_low_of_pv_and_start_bar > 1 ) continue;
		    //if(howMuchMove < 60) 	continue;		
			if(t1.GetHour() < 3) continue;		       // good++++
			if(t1.GetHour() > 19) continue;	                // good++++
			if(bar_sec_pv <= 60) continue;	     	                // 80 good++++
			if(bar_sec_pv > 1100) continue;		                // good+++
			if(bar_sec_st < 40) continue;	
			if(bar_sec_ed < 60) continue;	
			//if(t1.GetMinute() == 0) continue;	
			//if(diff_pv_gf_idx < 2) continue;	         // good
			//if(diff_pv_gf_idx > 80) continue;	           // good
			if(div_vol > 2 && div_vol < 2.2 ) continue;	  
			if(diff_pv_gf_idx > 20 && div_vol < 0.2 ) continue;	  
			if(diff_st_ed_idx > 6) continue;              // good
			
			//if(mean3 > mean_i_3) continue;	volum
			//if(mean3 < mean_i_3) continue;
			//if(mean3 < volum) continue;        // good
			//if(mean3 > volum) continue;  
			//if(div_mean < 0.2) continue;			
			if(div_mean > 1.12) continue;                // good
			//if(count_of_positive < 14)  continue; 		                // good+++
			//if(count_of_positive > 18)  continue;  		                // good+++
			
			if(countPos7 <= 9)  continue;  
			if(countPos7 >= 27)  continue;  
			
			if(countPos28 >= 58 && countPos28 <= 60)  continue;     
			if(countPos28 <= 48)  continue;  
			if(countPos28 >= 92)  continue;     //   87 is good to filter 20T profit
			
			if(div_cum_vol > 1.1 && div_cum_vol < 1.25) continue;        // 1.1 &1.25  good
			//if(div_cum_vol < 0.5) continue;     // good
			//if(div_cum_vol > 1) continue;        // good
		    //if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex < 80 ) continue;		
		 	//if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex > 450 ) continue;
			//if( vol[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] < 1000 ) continue;				
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < mean5/2 ) continue;	
			//if( vol[detectGoodSellOF->at(i).endBarIndex] < mean5*1 ) continue;	
			//if(meanDelta5*3 >  abs( delta[detectGoodSellOF->at(i).endBarIndex] )  ) continue;
			//if(meanMinDelta10 > -30 ) continue;			
			//if(meanMaxDelta10 > 100 ) //continue;					
			//if(sc.PriceValueToTicks(atrValue) > 1)  continue;			
			if(vol_per_sec[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] > f_vol_per_sec)  continue;			//1.4      // good
			if(vol_per_sec[detectDBWithGoodOF->at(i).indexOfGoodOfEndBar] < 0.18)  continue;	
			//if(detectGoodSellOF->at(i).endBarIndex - detectGoodSellOF->at(i).startBarIndex >= 3 && vol[detectGoodSellOF->at(i).endBarIndex] <= 1000) //continue;	
			if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) >= 20)  continue;		 //295	
			if(sc.PriceValueToTicks( vwap[endIdx]-sc.Close[endIdx] ) <= -40)  continue;		 //295	
			//if(sc.PriceValueToTicks( vwap[detectGoodSellOF->at(i).endBarIndex]-sc.Close[detectGoodSellOF->at(i).endBarIndex] ) >= 3)  continue;
			//if(cum_vol_day[detectGoodSellOF->at(i).endBarIndex] > 20000)  continue;
			if(cnt_good_sell >= 2) continue;	
			if(cnt_good_buy == 1) continue;	
			if(hmnPocAtLow >= 4) continue;
			
			
			
			//float volOfThisEndBar = detectGoodSellOF->at(i).volume_of_end_bar ;
			
			
			
	
			
	/*		int sumOFTouchHighOfStartIndex = 0;
			for(int k = endIdx-1 ; k >= startIdx+1; k--)
			{
				if(sc.Low[k] <= sc.High[startIdx]+sc.TickSize )   // if touch high of start bar index
				{
					sumOFTouchHighOfStartIndex++;
				}
			}
			//if(sumOFTouchHighOfStartIndex > 5 )  continue;  //3
		*/	
			// ########################################### End filter DT with Good Sell OF Here  ############################################
			
			
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
	
	
		
		
		
		// ################################## SIM ###############################################
		simTrade( sc , *fillterDBwithGoodBuyOF , *sim1s , tp_in_ticks) ;
		// ################################## SIM ###############################################
		
		
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


void simTrade(SCStudyInterfaceRef sc , std::vector<st_DoubleBottomWithGoodOF>& detectDBwithGoodBuyOF , std::vector<st_SimTrade>& detectTrade , int tp_in_ticks) 
{
	int tradeNumber = 0 ;
	float tp  ;
	
	
	detectTrade.clear() ;
	
	

	
		
	// loop in DB good buy OF
	for (int i = 0; i < detectDBwithGoodBuyOF.size(); i++)
	{
		int buyIndex = i;
		
		int entryIndex = detectDBwithGoodBuyOF[i].indexOfGoodOfEndBar ;
		float entryPrice = detectDBwithGoodBuyOF[i].highOfGoodOF ;
		float stopLoss = detectDBwithGoodBuyOF[i].lowOfGoodOF - sc.TickSize ;
		tp = entryPrice + tp_in_ticks*sc.TickSize  ;   bool passTP3 = false;
		float tp1 = entryPrice + TP_1*sc.TickSize  ;   bool passTP1 = false;
		float tp2 = entryPrice + TP_2*sc.TickSize  ;   bool passTP2 = false;
		
		
		int indexOfPattern ;
		int result ;
		
		// -------------- if have same end bar then go next i -------------------
		//if(entryIndex == detectDBwithGoodBuyOF[i-1].indexOfGoodOfEndBar && i >=1) continue;
		
		// loop from entry bar to end
		for(int j = entryIndex+1 ; j < sc.ArraySize-1; j++)
		{
			// if break high win
			if(sc.High[j] >= tp1 && passTP1 == false)
			{
	
				stopLoss = entryPrice + BE_1*sc.TickSize  ;
				passTP1 = true;				
				

			}
			
			if(sc.High[j] >= tp2 && passTP2 == false)
			{
							
				stopLoss = entryPrice + BE_2*sc.TickSize  ;
				passTP2 = true;	
				
			}
			
			if(sc.High[j] >= tp && passTP3 == false)
			{
							
				stopLoss = entryPrice + BE_3*sc.TickSize  ;
				passTP3 = true;	
			}
			
			if(sc.High[j] >= tp )
			{
				// win
				tradeNumber++;
				indexOfPattern = buyIndex;
				result = tp_in_ticks ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
			    
				break;	
				
			}
			
			
			// if breaK low loss
			if( sc.Low[j] <= stopLoss)
			{
				// loss
				tradeNumber++;
				indexOfPattern = buyIndex;
				result = sc.PriceValueToTicks(stopLoss - entryPrice) ;
				
				detectTrade.emplace_back(tradeNumber, indexOfPattern , result );
				break;		
			}
		}
		
		
	}
	
	
	
	
	SCString filePath = sc.DataFilesFolder() + "Sim DB with good OF Trades.txt";   		

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

    int totalProfit = 0 ;
	int numberWin = 0;
	int numberLoss = 0;
	
	
	
	// for ema200
	SCFloatArray EMAArray;
	sc.GetStudyArrayFromChartUsingID(2 , 1 , 0, EMAArray);
		
	// for good Sell OF
	SCFloatArray goodSellArray;
	sc.GetStudyArrayUsingID(9 , 0, goodSellArray);
		
	
	// for good Buy OF
	SCFloatArray goodBuyArray;
	sc.GetStudyArrayUsingID(9 , 1, goodBuyArray);
	
	// it is vol/sec
	SCFloatArray vol_per_sec;
	sc.GetStudyArrayUsingID(3, 35, vol_per_sec);
	
	
	// for atr
	SCFloatArray ATRArray;
	sc.GetStudyArrayFromChartUsingID(7 , 6 , 0, ATRArray);
	
	// it is totl vwap
	SCFloatArray vwap;
	sc.GetStudyArrayUsingID(12, 0, vwap);
	
		
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
	
	// it is bar duration
	SCFloatArray bar_dur_array;
	sc.GetStudyArrayUsingID(3, 29, bar_dur_array);
	
	// it is totl vol
	SCFloatArray vol;
	sc.GetStudyArrayUsingID(3, 12, vol);	
	
		
	
	for(int i = 0 ; i < detectTrade.size() ; i++)
	{
		int idx = detectTrade[i].indexOfPattern ;
		int pivotIndex = detectDBwithGoodBuyOF[idx].indexOfPivotLow;
		int startIndex = detectDBwithGoodBuyOF[idx].indexOfGoodOfStartBar ;
		int endIndex =  detectDBwithGoodBuyOF[idx].indexOfGoodOfEndBar ;
		int rs = detectTrade[i].result ;
		
		int diff_pv_gf_idx = startIndex  - pivotIndex  ;
		
		float hmm = findMaxGoUpBeforeBreakLow( sc , startIndex ,endIndex  );
		int howMuchMove = sc.PriceValueToTicks(hmm) ;
		
		int touchLow = findHowManyTouchLow( sc , pivotIndex ,startIndex) ;
		
		int i_mean = 7;   // 5
		float mean3 = meanVol( sc , endIndex, i_mean ) ; 			//5		
		float mean3_i_3 = meanVol( sc , endIndex-i_mean ,i_mean ) ; //5,5
		
		float div_mean = mean3/mean3_i_3 ;
		
		float mean_pivot_vol = meanVol( sc , pivotIndex, i_mean ) ; 
		float mean_lb_pivot_vol = meanVol( sc , pivotIndex-i_mean, i_mean ) ; 
		
		
		
		int RefDayChartIndex =	sc.GetNearestMatchForDateTimeIndex(2, endIndex);   // (chart number of m5 , sc.Index)
		
		
			
		
		float emaValue = EMAArray[RefDayChartIndex];
		
		
		int num = 10;              //20
		int cnt_good_sell = 0;
		for(int i = startIndex-1 ; i >= startIndex-num ; i--)
		{
			if(goodSellArray[i] == 1) cnt_good_sell++;
		}
		
				
		num = 10;              //20
		int cnt_good_buy = 0;
		for(int i = startIndex-1 ; i >= startIndex-num ; i--)
		{
			if(goodBuyArray[i] == 1) cnt_good_buy++;
		}
		
		
		float vps = vol_per_sec[endIndex] ; 
		
		// for atr
		int RefChartIndex =	sc.GetNearestMatchForDateTimeIndex(7, detectDBwithGoodBuyOF[i].indexOfGoodOfEndBar);   // (chart number of m5 , sc.Index)
	
		float atrValue = ATRArray[RefChartIndex];
	
		
		SCDateTime t1 = sc.BaseDateTimeIn[endIndex];   //detectDBwithGoodBuyOF[i].indexOfGoodOfEndBar
		SCString date = sc.DateTimeToString( t1 , FLAG_DT_COMPLETE_DATETIME);
		
			
	
		SCDateTime  bar_time = bar_dur_array[endIndex] ; 
		float bar_sec = bar_time.GetTimeInSeconds(); 
		
		// mean bar sec
		float total_bar_sec = 0;
		float mean_bar_sec = 0;
		for(int bs = 1 ; bs <= 20 ; bs++)
		{
			SCDateTime temp = bar_dur_array[endIndex-bs] ;			
			total_bar_sec += temp.GetTimeInSeconds() ;
		}
		mean_bar_sec = total_bar_sec/20;
		
		SCDateTime temp = bar_dur_array[pivotIndex] ;
		float bar_sec_pv = temp.GetTimeInSeconds() ;
		
		temp = bar_dur_array[startIndex] ;
		float bar_sec_st = temp.GetTimeInSeconds() ;
				
		
		 //delta delta_chg max_delta min_delta percent_delta
		int idx_end_bar = endIndex ;
		int idx_st_bar = startIndex ;
		int count_of_positive = 0;
	/*	for(int k = idx_end_bar ; k > idx_end_bar-7 ; k--)
		{
			if(delta[k] > 0 ) count_of_positive++ ;
			if(delta_chg[k] > 0 ) count_of_positive++ ;
			if(max_delta[k] > 0 ) count_of_positive++ ;
			if(min_delta[k] > 0 ) count_of_positive++ ;
			if(percent_delta[k] > 0 ) count_of_positive++ ;
		}*/
		
		int countPos7 = count_pos_table( sc , endIndex , 7 ) ;
		
		int countPos2 = count_pos_table( sc , endIndex , 28 ) ;	//20  25  24
			
	
        int i_vol = 9;	
		float cum_vol = cummulateVol( sc , idx_end_bar ,i_vol) ;
		float cum_vol_i_10 = cummulateVol( sc , idx_end_bar-i_vol ,i_vol) ;
		float div_cum_vol = cum_vol/cum_vol_i_10 ;
		
		int hmnPocAtLow = findHowManyDownCandleWithPocAtLow(sc ,idx_st_bar , 9 ) ;	
		
		int cumDel = cummulateDelta( sc , idx_end_bar , 10) ;
		
		int hmnBreakPrevLow =  findBreakLowOfPrevBar( sc ,idx_end_bar , 30 )  ;
		
		int num_touch_vwap = findHowManyTouchVwap( sc , idx_end_bar , 50 );   //30
		
		int numMaxDelta = 9  ;
		float meanMax5 = meanMaxDelta(  sc , idx_end_bar , numMaxDelta ) ;
		float meanMax5_num = meanMaxDelta(  sc , idx_end_bar-numMaxDelta , numMaxDelta ) ;
		
		
		float cumMaxDelta5 = cummulateMaxDelta( sc , idx_end_bar , numMaxDelta)  ;
		float cumMaxDelta5_num = cummulateMaxDelta( sc , idx_end_bar-numMaxDelta , numMaxDelta)  ;
		float div_cum_max_delta = cumMaxDelta5/cumMaxDelta5_num ; 
		
		
		float cumMinDelta5 = cummulateMinDelta(sc , idx_end_bar , numMaxDelta) ;
		float cumMinDelta5_num = cummulateMinDelta(sc , idx_end_bar-numMaxDelta , numMaxDelta) ;
		float div_cum_min_delta = cumMinDelta5/cumMinDelta5_num ; 
		
			
		outputFile << "Trade: " << detectTrade[i].tradeNumber ;
		outputFile << ", pv idx : " << pivotIndex ;
		outputFile << ", st idx : " << startIndex ;
		outputFile << ", ed idx : " << endIndex ;
		outputFile << ", st vol : " << vol[startIndex]  ;
		outputFile << ", vol-1 : " << vol[endIndex-1] ;
		outputFile << ", diff pv gf : " << diff_pv_gf_idx ;   
		outputFile << ", vps : " << vps ;
		//outputFile << ", div max : " << meanMax5/meanMax5_num ;
		//outputFile << ", div vol : " << vol[endIndex] / vol[endIndex-1] ;
		outputFile << ", div vol : " << vol[endIndex] / vol[startIndex] ;
		//outputFile << ", break low : " << hmnBreakPrevLow ;
		//outputFile << ", touch vwap : " << num_touch_vwap ;   
		//outputFile << ", diff max delta  : " << max_delta[idx_end_bar] - max_delta[idx_st_bar] ;
		//outputFile << ", diff min delta  : " << min_delta[idx_end_bar] - min_delta[idx_st_bar] ; ;
		//outputFile << ", mean i : " << mean3 ;
		//outputFile << ", mean i-3 : " << mean3_i_3 ;
		outputFile << ", mean pv : " << mean_pivot_vol ;
		outputFile << ", mean pv lb : " << mean_lb_pivot_vol ; 
		outputFile << ", bar sec pv : " << bar_sec_pv ; 
		outputFile << ", bar sec st : " << bar_sec_st ; 
		outputFile << ", bar sec ed : " << bar_sec ; 
		outputFile << " , vwap: " << sc.PriceValueToTicks( vwap[idx_end_bar]-sc.Close[idx_end_bar] )  ;
		//outputFile << " , atr: " << sc.PriceValueToTicks(atrValue)  ;
		outputFile << ", div mean : " << div_mean ;
		//outputFile << ", delta_chg : " << delta_chg[idx_end_bar] ; 
		//outputFile << ", ema200 : " << sc.PriceValueToTicks( sc.Close[endIndex] - emaValue) ;
		//outputFile << ", bar sec : " << bar_sec ;		   
		outputFile << ", mean bar sec : " << mean_bar_sec ;		   
		outputFile << ", count pos : " << countPos7  ; 
		outputFile << ", count pos2 : " << countPos2  ; 
		//outputFile << ", touch low : " << touchLow  ;  
		//outputFile << ", hmn poc low : " << hmnPocAtLow  ;  
		//outputFile << ", cum del : " << cumDel ;  
		//outputFile << ", cnt GS : " << cnt_good_sell ;
		//outputFile << ", cnt GB : " << cnt_good_buy ;
		//outputFile << ", cum vol : " << cum_vol ;
		//outputFile << ", cum vol - i : " << cum_vol_i_10 ;
		outputFile << ", div cum vol : " << div_cum_vol ;
		//outputFile << ", div cum min : " << div_cum_min_delta ;
		outputFile << " , result: " << rs ;
		outputFile << " , best move : " << howMuchMove << "T" ;
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
	outputFile << "number Win: " << numberWin << "\n" ;
	outputFile << "number Loss: " << numberLoss << "\n" ;
	outputFile << "Total profit: " << totalProfit << "\n" ;
	
    
	// Close the file
	outputFile.close();	
	
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

float meanMaxDelta( SCStudyInterfaceRef sc , int index , int lookback )
{
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta); 
	float sum=0;
	
	for(int i = index ; i >= index - lookback+1 ; i-- )
	{
		sum += abs( max_delta[i] );
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

int cummulateMaxDelta(SCStudyInterfaceRef sc , int index , int lookback)
{
	// Get max delta
	SCFloatArray max_delta;
	sc.GetStudyArrayUsingID(3, 7, max_delta); 
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += max_delta[i];
	}
	return sum;
	
}


int cummulateMinDelta(SCStudyInterfaceRef sc , int index , int lookback)
{
	// Get min delta
	SCFloatArray min_delta;
	sc.GetStudyArrayUsingID(3, 8, min_delta);
	float sum=0;
	
	for(int i = index ; i > index - lookback ; i-- )
	{
		sum += min_delta[i];
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

int findBreakLowOfPrevBar(SCStudyInterfaceRef sc ,int index , int lookback  )
{
	int count = 0;
	int endIndex = index-lookback;
	
	if(endIndex < 0) return 1000;
	
	for(int i = index ; i >= endIndex ; i--)
	{
		if(sc.Low[i] < sc.Low[i-1]) count++ ;
	}
	return count;
	
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
















