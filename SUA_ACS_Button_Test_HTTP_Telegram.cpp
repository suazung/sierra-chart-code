







#include "sierrachart.h"



SCDLLName("ACS_BUTTON_TEST_HTTP")



void sendHttpPostMessage( SCStudyInterfaceRef sc ,int uniqNumber , int signal , SCString m1TimeString , SCString currentTimeString)  ;

bool get_M1Time_currentTime_String(SCStudyInterfaceRef sc ,int M1ChartNumber , int IndexOfSL , int signal , 
									SCString& m1TimeString , SCString& currentTimeString  )   ;
									
									
									
SCSFExport scsf_ACSButtonTestHttp(SCStudyGraphRef sc)
{
    
   
    
    SCString m1TimeString , currentTimeString , textToTelegram;
	
	
	
	int BUY_SIGNAL = 1 ;
	int SELL_SIGNAL = -1 ;
	
	int M1ChartNumber = 7 ;	
	
	int TEST_CODE_BUY = 12000  ;
	int TEST_CODE_SELL = 14000  ;
	
	int keyUniqueValue = 1;
	int persistentUniqueValue = sc.GetPersistentInt(keyUniqueValue);   // 1 is key.
	
	
	
    if (sc.SetDefaults)
    {
        sc.GraphName = "ACS Button Test HTTP";
        sc.StudyDescription = "Test HTTP and Telegram.";
        sc.AutoLoop = 0;
        sc.UpdateAlways = 1;

        
        return;
    }
    
	
	// ##############################       processing here        ###################################
	
	if (sc.IsFullRecalculation || sc.LastCallToFunction || sc.HideStudy)
	{
		
		
		persistentUniqueValue = 0;
		sc.SetPersistentInt(keyUniqueValue, persistentUniqueValue);		
			
		// Study is being removed nothing more to do
		if (sc.LastCallToFunction || sc.HideStudy)
			return;
	}
	
	
	
	
    if (sc.MenuEventID == 1)
    {
        const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
        if (ButtonState == 1)
        {
       
			sc.AddMessageToLog("Buy clicked! ...", 0);
			
			sc.SetCustomStudyControlBarButtonEnable(1, false); 
			
			
			persistentUniqueValue++ ;
				
			if(persistentUniqueValue > 500)
				persistentUniqueValue = 0;
				
			sc.SetPersistentInt(keyUniqueValue, persistentUniqueValue);	
					
					
			int uniqNumber = persistentUniqueValue + TEST_CODE_BUY  ;
			
			
			bool isTimeCorrect = get_M1Time_currentTime_String( sc ,M1ChartNumber , sc.ArraySize-1 ,  BUY_SIGNAL , m1TimeString , currentTimeString ) ;
					
					
			sendHttpPostMessage( sc, uniqNumber ,  BUY_SIGNAL ,  m1TimeString , currentTimeString) ;
						
			
			textToTelegram.Format("TEST BUY NOW!                                                ");
	
	
			 // Trigger the alert
			sc.SetAlert(1 , textToTelegram );
			
		
        }
    
    }
	
	
	if (sc.MenuEventID == 2 )   // test sell button = custom study button 2
    {
        const int ButtonState = (sc.PointerEventType == SC_ACS_BUTTON_ON) ? 1 : 0;
        if (ButtonState == 1)
        {
       
			sc.AddMessageToLog("Sell clicked! ...", 0);
			
			sc.SetCustomStudyControlBarButtonEnable( 2, false);     // custom study button 2
			
			
			persistentUniqueValue++ ;
				
			if(persistentUniqueValue > 500)
				persistentUniqueValue = 0;
				
			sc.SetPersistentInt(keyUniqueValue, persistentUniqueValue);	
					
					
			int uniqNumber = persistentUniqueValue + TEST_CODE_SELL  ;
			
			
			bool isTimeCorrect = get_M1Time_currentTime_String( sc ,M1ChartNumber , sc.ArraySize-1 ,  SELL_SIGNAL , m1TimeString , currentTimeString ) ;
					
					
			sendHttpPostMessage( sc, uniqNumber ,  SELL_SIGNAL ,  m1TimeString , currentTimeString) ;
						
			
			textToTelegram.Format("TEST SELL NOW!                                               ");
	
	
			 // Trigger the alert
			sc.SetAlert(2 , textToTelegram );
			
		
        }
    
    }
	
	
	
	
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






