











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
	
	int TEST_CODE_BUY = 500000  ;
	int TEST_CODE_SELL = 700000  ;
	
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




// ############################################ python #################################################################


from fastapi import FastAPI
from pydantic import BaseModel
import uvicorn
import zmq
import time

app = FastAPI()

# สร้าง context และ socket
context = zmq.Context()
socket = context.socket(zmq.REQ)  # ใช้ REQ (Request) socket
socket.connect("tcp://localhost:5557")  # เชื่อมต่อไปยัง MT5


latest_signal = {
    "uniq": None,
    "signal": None,
    "m1time": None,
    "currenttime": None
}

    
class Signal(BaseModel):
    uniq: int
    signal: int
    m1time: str
    currenttime: str




@app.post("/simplesignal")
async def simplesignal(sig: Signal):
    global latest_signal
    latest_signal = {
        "uniq": sig.uniq,
        "signal": sig.signal,
        "m1time": sig.m1time,
        "currenttime": sig.currenttime
    }
    print(f"Received signal - unique number: {sig.uniq}, signal: {sig.signal}, M1 Time: {sig.m1time}, Now Time: {sig.currenttime}")
    
    try:
        # ส่งค่าตัวแปร a และ b ไปยัง MT5
        message = f"{sig.uniq},{sig.signal},{sig.m1time},{sig.currenttime}"
        print(f"Sending message: {message}")
        socket.send_string(message)

        # รับผลลัพธ์ที่ MT5 ส่งกลับ
        result = socket.recv_string()
        print(f"ผลลัพธ์ที่ได้รับจาก MT5: {result}")

        # รอ 5 วินาที
        #time.sleep(5)

        # เพิ่มค่า i
        #i += 1
    except Exception as e:
        print(f"เกิดข้อผิดพลาด: {e}")
        
    
    return {"status": "success", "received_signala": latest_signal}




@app.get("/simplesignal")
async def get_signal():
    return {"status": "ready", "latest_signal": latest_signal}



if __name__ == "__main__":
    uvicorn.run(app, host="0.0.0.0", port=8000)
    








// ############################################ MT5 #################################################################






#include <Trade/Trade.mqh>
#include <Zmq/Zmq.mqh>


int diffTimeSierraChartAndMT5(datetime currentTimeSierraChart  ) ;

input double Lot = 0.1;
input string symbol = "GBPUSD" ;

int i = 0;  // ตัวแปรที่ใช้เพิ่มค่า
Context context("mt5server");
Socket socket(context, ZMQ_REP);  // ใช้ REP (Reply) socket

CTrade trade;

int digits ;  
double onePips ;
   
input bool allowTrade = true ;    
input bool allowNewsTrade = false ;   
input bool allowHedgeTrade = false ; 

input int portNumber = 5557 ;

int buy_count = -1;   
int sell_count = -1;                   //#1 initialize counts
    
void OnInit()
{
    string portString = "tcp://*:" + IntegerToString(portNumber)  ;
    
    //if (!socket.bind("tcp://*:5557"))  // ผูก socket 
    if (!socket.bind(portString)  )   // ผูก socket 
    {
        Print("ไม่สามารถผูก socket กับพอร์ต 5557");
        
    }
    else
    {
        Print("MT5 server started and listening on port 5557");
    }
    
    int digits = SymbolInfoInteger(symbol, SYMBOL_DIGITS);
     //int floatOfSybol = digits-1 ;
    double onePips = MathPow(10, -1*digits)*10 ;
    onePips = NormalizeDouble(onePips,digits); 
    Print("digit = ",digits , " | 1 pips = " , onePips) ;
}

void OnTick()
{
    ZmqMsg request;
    if (socket.recv(request))  // ตรวจสอบว่ามีข้อความจาก Python หรือไม่
    {
        // แปลงข้อความที่รับเป็นสตริง
        string request_str = request.getData();
        Print("Received data: ", request_str);

        string arr[];
        StringSplit(request_str, ',', arr);

        if (ArraySize(arr) == 4)
        {
            // แปลงข้อความเป็นตัวเลข
            int uniq = StringToInteger(arr[0]);
            int signal = StringToInteger(arr[1]);
            string m1TimeStringSierraChart = arr[2] ;
            string currentTimeStringSierraChart = arr[3] ;

            datetime m1TimeSierraChart = StringToTime(m1TimeStringSierraChart) ;
            datetime currentTimeSierraChart = StringToTime(currentTimeStringSierraChart) ;           
            
            int t1 = diffTimeSierraChartAndMT5(currentTimeSierraChart) ;
            
            datetime m1TimeForMT5 = m1TimeSierraChart - t1 ;
            
            int indexM1 = iBarShift(symbol, PERIOD_M1, m1TimeForMT5);
            double lowM1 = iLow(symbol, PERIOD_M1, indexM1);
            double highM1 = iHigh(symbol, PERIOD_M1, indexM1);
            
            
            // คำนวณผลลัพธ์
            int result = uniq*signal*10 ;

             digits = SymbolInfoInteger(symbol, SYMBOL_DIGITS);
             onePips = MathPow(10, -1*digits)*10 ;
             onePips = NormalizeDouble(onePips,digits);
            
           
            
            
            if(uniq >= 500000 )
            {               
                                               
               if(signal == 1  )
               {
                  double last_price = SymbolInfoDouble(symbol, SYMBOL_ASK);
                  double sl = lowM1 - onePips ;
                  double tp = last_price + 30*onePips ;
                  sl = NormalizeDouble(sl,digits); 
                  tp = NormalizeDouble(tp,digits); 
                     
                  openPosition( symbol ,  0.01 ,  signal ,  sl ,  tp , allowTrade ,  allowHedgeTrade ,  allowNewsTrade) ;
                  
                  
               }
               else if(signal == -1 )
               {
                  double last_price = SymbolInfoDouble(symbol, SYMBOL_BID);
                  double sl = highM1 + onePips ;
                  double tp = last_price - 30*onePips ;
                  sl = NormalizeDouble(sl,digits); 
                  tp = NormalizeDouble(tp,digits); 
                     
                  openPosition( symbol ,  0.01 ,  signal ,  sl ,  tp , allowTrade ,  allowHedgeTrade ,  allowNewsTrade) ;
                  
                  
               }
               else if(signal == 1 && allowTrade == false )
               {
                  Print("buy signal");
                  Print("HTTP and Socket work fine !!!");
               }
               else if(signal == -1 && allowTrade == false )
               {
                  Print("sell signal");
                  Print("HTTP and Socket work fine !!!");
               }
            
            }            
            else   //    uniq < 500000 
            {
               if(signal == 1)
               {
                  double sl = lowM1 - onePips ;
                  sl = NormalizeDouble(sl,digits); 
                  trade.Buy(Lot,symbol , 0  , sl) ;
               }
               else if(signal == -1)
               {
                  double sl = highM1 + onePips ;
                  sl = NormalizeDouble(sl,digits); 
                  trade.Sell(Lot,symbol , 0 , sl) ;
               }            
            
            }
           
            
            // ส่งผลลัพธ์กลับไปยัง Python
            ZmqMsg reply(IntegerToString(result));
            socket.send(reply);
            
            Print("Sent result: ", IntegerToString(result));
            Print("signal num: ", IntegerToString(signal));
            Print("M1 mt5 time: ", m1TimeForMT5 );
            Print("M1 Low:",DoubleToString(lowM1) );
            Print("M1 High:",DoubleToString(highM1) );
            Print("current time: ", currentTimeStringSierraChart );
            
            datetime currentTimeMT5 = TimeCurrent();
            int timeDiff = currentTimeMT5 - (currentTimeSierraChart - t1) ;
            Print("time diff :",IntegerToString(timeDiff) );
            //Print("t1:",IntegerToString(t1) );
        }
        else
        {
            Print("ข้อมูลที่ได้รับไม่ถูกต้อง: ", request_str);
        }
    }
    
    
       
    
   
   //getNumberOfPositionNow(symbol , buy_count,sell_count);
   
   //Print("\n\n buy = ",buy_count , " | sell = " ,sell_count );



}


void OnDeinit(const int reason)
{
   Print("MT5 server stopped and resources freed.");
}


int diffTimeSierraChartAndMT5(datetime currentTimeSierraChart  )
{
   
   //datetime currentTimeSierraChart = StringToTime(currentTimeString) ;
   datetime currentTimeMT5 = TimeCurrent();
   
   int t1 = currentTimeSierraChart-currentTimeMT5 ;
   
   
   int secondOfHour = 3600;
   int tolorent = 100 ;
   int pos_or_neg = 0;
   
   if(t1 > 0) 
      pos_or_neg = 1;
   else if(t1 < 0)
      pos_or_neg = -1; 
      
   t1 = MathAbs(t1) ;
      
   if( (t1 <= 0+tolorent) && (t1 > 0-tolorent) )    // 3600
      return 0*pos_or_neg ;
   else if( (t1 <= secondOfHour+tolorent) && (t1 > secondOfHour-tolorent) )    // diff 1hr
      return secondOfHour*pos_or_neg ;
   else if( (t1 <= 2*secondOfHour+tolorent) && (t1 > 2*secondOfHour-tolorent) ) // diff 2hr
      return 2*secondOfHour*pos_or_neg ;
   else if( (t1 <= 3*secondOfHour+tolorent) && (t1 > 3*secondOfHour-tolorent) )  // diff 3hr
      return 3*secondOfHour*pos_or_neg ;
   else if( (t1 <= 4*secondOfHour+tolorent) && (t1 > 4*secondOfHour-tolorent) )  // diff 4hr
      return 4*secondOfHour*pos_or_neg ;
   else if( (t1 <= 5*secondOfHour+tolorent) && (t1 > 5*secondOfHour-tolorent) )  // diff 5hr
      return 5*secondOfHour*pos_or_neg ;
   else if( (t1 <= 6*secondOfHour+tolorent) && (t1 > 6*secondOfHour-tolorent) )  // diff 6hr
      return 6*secondOfHour*pos_or_neg ;
   else if( (t1 <= 7*secondOfHour+tolorent) && (t1 > 7*secondOfHour-tolorent) )  // diff 7hr
      return 7*secondOfHour*pos_or_neg ;
   else if( (t1 <= 8*secondOfHour+tolorent) && (t1 > 8*secondOfHour-tolorent) )  // diff 8hr
      return 8*secondOfHour*pos_or_neg ;
   else if( (t1 <= 9*secondOfHour+tolorent) && (t1 > 9*secondOfHour-tolorent) )  // diff 9hr
      return 9*secondOfHour*pos_or_neg ;
   else if( (t1 <= 10*secondOfHour+tolorent) && (t1 > 10*secondOfHour-tolorent) )  // diff 10hr
      return 10*secondOfHour*pos_or_neg ;
   else if( (t1 <= 11*secondOfHour+tolorent) && (t1 > 11*secondOfHour-tolorent) )  // diff 11hr
      return 11*secondOfHour*pos_or_neg ;
   else if( (t1 <= 12*secondOfHour+tolorent) && (t1 > 12*secondOfHour-tolorent) )  // diff 12hr
      return 12*secondOfHour*pos_or_neg ;
   else if( (t1 <= 13*secondOfHour+tolorent) && (t1 > 13*secondOfHour-tolorent) )  // diff 13hr
      return 13*secondOfHour*pos_or_neg ;
   else if( (t1 <= 14*secondOfHour+tolorent) && (t1 > 14*secondOfHour-tolorent) )  // diff 14hr
      return 14*secondOfHour*pos_or_neg ;
   else if( (t1 <= 15*secondOfHour+tolorent) && (t1 > 15*secondOfHour-tolorent) )  // diff 15hr
      return 15*secondOfHour*pos_or_neg ;
   else if( (t1 <= 16*secondOfHour+tolorent) && (t1 > 16*secondOfHour-tolorent) )  // diff 16hr
      return 16*secondOfHour*pos_or_neg ;
   else if( (t1 <= 17*secondOfHour+tolorent) && (t1 > 17*secondOfHour-tolorent) )  // diff 17hr
      return 17*secondOfHour*pos_or_neg ;
   else if( (t1 <= 18*secondOfHour+tolorent) && (t1 > 18*secondOfHour-tolorent) )  // diff 18hr
      return 18*secondOfHour*pos_or_neg ;
   else if( (t1 <= 19*secondOfHour+tolorent) && (t1 > 19*secondOfHour-tolorent) )  // diff 19hr
      return 19*secondOfHour*pos_or_neg ;
   else if( (t1 <= 20*secondOfHour+tolorent) && (t1 > 20*secondOfHour-tolorent) )  // diff 20hr
      return 20*secondOfHour*pos_or_neg ;
   else if( (t1 <= 21*secondOfHour+tolorent) && (t1 > 21*secondOfHour-tolorent) )  // diff 21hr
      return 21*secondOfHour*pos_or_neg ;
   else if( (t1 <= 22*secondOfHour+tolorent) && (t1 > 22*secondOfHour-tolorent) )  // diff 22hr
      return 22*secondOfHour*pos_or_neg ;
   else if( (t1 <= 23*secondOfHour+tolorent) && (t1 > 23*secondOfHour-tolorent) )  // diff 23hr
      return 23*secondOfHour*pos_or_neg ; 
   else if( (t1 <= 24*secondOfHour+tolorent) && (t1 > 24*secondOfHour-tolorent) )  // diff 24hr
      return 24*secondOfHour*pos_or_neg ;                                                                  
  
   return -1;
   
}


void getNumberOfPositionNow(string mySymbol , int& buy_count , int& sell_count)
{
   buy_count = 0;
   sell_count= 0;
   
   for(int i = PositionsTotal() - 1 ; i >= 0 ; i--)   //#2 not i <= 0 
   {
      PositionGetTicket(i);                            //#3 Missing PositionGetTicket(i);    
     
       if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_BUY
          && PositionGetString(POSITION_SYMBOL) == mySymbol)            
       {
         buy_count++;
       }
       if(PositionGetInteger(POSITION_TYPE) == POSITION_TYPE_SELL
          && PositionGetString(POSITION_SYMBOL) == mySymbol)    
       {
        sell_count++;
       }

   } // for loop end




}



void openPosition(string symbol , double lot , int signal , double sl , double tp ,
                 bool allowTrade , bool allowHedgeTrade , bool allowNewsTrade)
{
   
   int buy_count = -1 ;
   int sell_count = -1 ;
   
    getNumberOfPositionNow(symbol , buy_count, sell_count);

   if(signal == 1 && allowTrade )
   {
               
      if(sell_count <= 0 ||
         (sell_count > 0 && allowHedgeTrade)  )
      {                     
         trade.Buy(lot,symbol , 0  , sl , tp) ;
      }
      else 
      {
         Print("having sell now and hedging not allow !!");
      }
      
      
   }
   else if(signal == -1 && allowTrade )
   {
          
      if(buy_count <= 0 ||
         (buy_count > 0 && allowHedgeTrade)  )
      {                     
         trade.Sell(lot,symbol , 0 , sl) ;
      }
      else 
      {
         Print("having buy now and hedging not allow !!");
      }                  
      
   }
   else if(signal == 1 && allowTrade == false )
   {
      Print("buy signal");
      Print("HTTP and Socket work fine !!!");
   }
   else if(signal == -1 && allowTrade == false )
   {
      Print("sell signal");
      Print("HTTP and Socket work fine !!!");
   }

}            
 

































