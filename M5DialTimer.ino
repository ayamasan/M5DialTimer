#include <M5Dial.h>

time_t ttset = 0;
time_t ttold = 0;
int timer = 0;
int timeron = 0;
int sx, sy;
int addtime = 1;
int alarming = 0;

int bkcolor = BLACK;

long oldPosition = -999;

// 初期設定
void setup() {
	// put your setup code here, to run once:
	
	auto cfg = M5.config();
	M5Dial.begin(cfg, true, true);
	
	M5Dial.Display.fillScreen();
	M5Dial.Display.setTextColor(GREEN, bkcolor);
	M5Dial.Display.setTextDatum(middle_center);
	M5Dial.Display.setTextFont(7);
	M5Dial.Display.setTextSize(0.5);
	
	sx = M5Dial.Display.width() / 2;
	sy = M5Dial.Display.height() / 2;
	
	// RTC
	auto dt = M5Dial.Rtc.getDateTime();
	ttset = dt2tt(dt, 1);
	ttold = ttset;
	
	M5Dial.Display.fillScreen(bkcolor);
	M5Dial.Display.setTextFont(&fonts::Orbitron_Light_32);
	M5Dial.Display.setTextColor(WHITE, bkcolor);
	M5Dial.Display.setTextSize(0.5);
	M5Dial.Display.drawString("TIMER", sx, sy-40);
	M5Dial.Display.fillRect(sx+8, sy+30, 60, 4, DARKCYAN);
	
}

// m5::rtc_datetime_t -> time_t 変換
time_t dt2tt(m5::rtc_datetime_t dt, int pp)
{
	struct tm tm;
	tm.tm_year  = dt.date.year - 1900;    // 年 [1900からの経過年数]
	tm.tm_mon   = dt.date.month - 1;   // 月 [0-11] 0から始まることに注意
	tm.tm_mday  = dt.date.date;    // 日 [1-31]
	tm.tm_wday  = dt.date.weekDay; // 曜日 [0:日 1:月 ... 6:土]
	tm.tm_hour  = dt.time.hours;   // 時 [0-23]
	tm.tm_min   = dt.time.minutes; // 分 [0-59]
	tm.tm_sec   = dt.time.seconds; // 秒 [0-61]
	tm.tm_isdst = -1;              // 夏時間フラグ
	
	if(pp != 0){
		Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n"
		, tm.tm_year + 1900
		, tm.tm_mon  + 1
		, tm.tm_mday
		, tm.tm_hour
		, tm.tm_min 
		, tm.tm_sec 
		);
	}
	
	time_t tt = mktime(&tm) + (8 * 60 * 60);
	return(tt);
}

// メインループ
void loop() {
	// put your main code here, to run repeatedly:
	M5Dial.update();
	
	int min;
	int sec;
	char buf[100];
	
	if(M5Dial.BtnA.wasPressed()){
		if(alarming == 0 && timeron == 0){
			// カウントダウンスタート
			alarming = 0;
			auto dt = M5Dial.Rtc.getDateTime();
			ttold = dt2tt(dt, 0);
			ttset = ttold + timer;
			timeron = 1;
			Serial.println("TIMER START");
		}
		else{
			Serial.println("TIMER STOP");
			// アラームストップ
			alarming = 0;
			timeron = 0;
			
			min = (timer / 60);
			sec = (timer % 60);
			M5Dial.Display.setTextSize(1);
			M5Dial.Display.setTextColor(GREEN, bkcolor);
			M5Dial.Display.setTextFont(7);
			sprintf(buf, "%02d:%02d", min, sec);
			M5Dial.Display.drawString(buf, sx, sy);
			drawarc(min, sec, 0);
		}
	}
	
	// タッチで分秒切替
	auto t = M5Dial.Touch.getDetail();
	if(t.state == 3){  // touch_begin
		// 分秒切替
		if(addtime == 1){
			addtime = 60;
			M5Dial.Display.fillRect(sx+8, sy+30, 60, 4, bkcolor);
			M5Dial.Display.fillRect(sx-68, sy+30, 60, 4, CYAN);
		}
		else{
			addtime = 1;
			M5Dial.Display.fillRect(sx+8, sy+30, 60, 4, DARKCYAN);
			M5Dial.Display.fillRect(sx-68, sy+30, 60, 4, bkcolor);
		}
	}
	
	// ダイアル処理、設定時間
	if(timeron == 0){
		long newPosition = M5Dial.Encoder.read();
		int redraw = 0;
		if(newPosition > oldPosition){
			if(timer < 3599){
				if((timer + addtime) < 3600){
					timer += addtime;  // +1分
					redraw = 1;
				}
				else{
					timer = 3599;
					redraw = 1;
				}
			}
			oldPosition = newPosition;
		}
		else if(newPosition < oldPosition){
			if(timer > 0){
				if((timer - addtime) >= 0){
					timer -= addtime;  // -1分
					redraw = 1;
				}
				else{
					timer = 0;
					redraw = 1;
				}
			}
			oldPosition = newPosition;
		}
		if(redraw != 0){
			// 描画
			min = (timer / 60);
			sec = (timer % 60);
			M5Dial.Display.setTextSize(1);
			M5Dial.Display.setTextColor(GREEN, bkcolor);
			M5Dial.Display.setTextFont(7);
			sprintf(buf, "%02d:%02d", min, sec);
			M5Dial.Display.drawString(buf, sx, sy);
			// ダイアル開店時のみ音を鳴らす
			M5Dial.Speaker.tone(1000, 20);
			
			drawarc(min, sec, 0);
		}
	}
	
	auto dt = M5Dial.Rtc.getDateTime();
	time_t tt = dt2tt(dt, 0);
	
	// 20億超えと+-60秒以上とマイナスは無視する
	if(tt != ttold){
		if(tt > 2000000000){
			tt = ttold;
		}
		else if(tt < 0){
			tt = ttold;
		}
		else if(((tt - ttold) > 60) || ((tt - ttold) < -60)){
			tt = ttold;
		}
	}
	
	if(tt > ttold && timeron != 0){
		// 時間更新
		if((ttset - tt) >= 0){
			min = ((ttset - tt) / 60);
			sec = ((ttset - tt) % 60);
			M5Dial.Display.setTextSize(1);
			M5Dial.Display.setTextColor(GREEN, bkcolor);
			M5Dial.Display.setTextFont(7);
			sprintf(buf, "%02d:%02d", min, sec);
			M5Dial.Display.drawString(buf, sx, sy);
			Serial.println(buf);
			
			drawarc(min, sec, 1);
		}
		else{
			if(alarming == 0){
				// アラーム開始
				
			}
			alarming = 1;
			M5Dial.Speaker.tone(4000, 100);
			delay(100);
			M5Dial.Speaker.tone(4000, 100);
		}
		ttold = tt;
	}
	else{
		delay(20);
	}
	
}


// 時間グラフィック表示（円弧）
void drawarc(int min, int sec, int mode)
{
	int col1;
	int col2;
	int rmin;
	int rsec;
	
	if(mode == 0){
		col1 = DARKCYAN;
		col2 = CYAN;
	}
	else{
		col1 = DARKGREEN;
		col2 = GREEN;
	}
	
	rmin = min * 6;
	rsec = sec * 6;
	if(rsec < 90){
		M5Dial.Display.fillArc(sx, sy, 102, 110, 270, 270+rsec, col1);
		M5Dial.Display.fillArc(sx, sy, 102, 110, 270+rsec, 360, BLACK);
		M5Dial.Display.fillArc(sx, sy, 102, 110, 0, 270, BLACK);
	}
	else{
		M5Dial.Display.fillArc(sx, sy, 102, 110, 270, 360, col1);
		M5Dial.Display.fillArc(sx, sy, 102, 110, 0, rsec-90, col1);
		M5Dial.Display.fillArc(sx, sy, 102, 110, rsec-90, 270, BLACK);
	}
	if(rmin < 90){
		M5Dial.Display.fillArc(sx, sy, 112, 120, 270, 270+rmin, col2);
		M5Dial.Display.fillArc(sx, sy, 112, 120, 270+rmin, 360, BLACK);
		M5Dial.Display.fillArc(sx, sy, 112, 120, 0, 270, BLACK);
	}
	else{
		M5Dial.Display.fillArc(sx, sy, 112, 120, 270, 360, col2);
		M5Dial.Display.fillArc(sx, sy, 112, 120, 0, rmin-90, col2);
		M5Dial.Display.fillArc(sx, sy, 112, 120, rmin-90, 270, BLACK);
	}
}

