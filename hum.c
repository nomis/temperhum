#include <stdio.h>
#include <string.h>
#include <math.h>

#include "comms.h"

double Bin2Dec(const char *s) {
    double lDec = 0.0;

    if (s == NULL || strlen(s) == 0)
        s = "0";

    while (*s != '\0')
    {
        if (*s == '1')
            lDec += pow(2.0, strlen(s) - 1);
        s++;
    }
    return lDec;
}


static double lasttemp = 0;

double ReadSHT(char TH) {
			char buf[20];
			int xxx=0, bad=0;

	double tempdata=0;

			temper_switch(1, 0);
			temper_switch(0, 1);

			if (TH == 'T')
				temper_write(0x03, 8);
			else if (TH == 'H')
				temper_write(0x05, 8);

			xxx = temper_wait(500);
			if (xxx < 0) {
				bad = 1;
				printf("bad 1\n");
			} else {
				printf("waited %d\n", xxx);
			}

    memset(buf, '\0', sizeof(buf));
    int i;

    for (i = 0; i < 16; i++) {
        int s = temper_get();

        buf[i] = (s == 0 ? '0' : '1');
        if (i != 15)
			temper_write(1, 1);

        if (i == 7) {
			temper_switch(1, 0);
			temper_write(0, 1);
        }
    }

			if (TH == 'T')
			{
				printf("\t\t\t\t\t\tT %s\n", buf);
// 0123 4567 8901 2345
//   XX XXXX XXXX XXXX
//				str_temp = Strings.Right(str_data, 14);
//   XX XXXX
//				str_msb = Strings.Left(str_temp, 6);
//           XXXX XXXX
//				str_lsb = Strings.Right(str_temp, 8);


    char str_msb[7];
    char str_lsb[9];

    memcpy(str_msb, buf+2, 6);
    str_msb[6] = '\0';

    memcpy(str_lsb, buf+8, 8);
    str_lsb[8] = '\0';

    double msb = Bin2Dec(str_msb);
    double lsb = Bin2Dec(str_lsb);
   tempdata = (msb * 256.0) + lsb;

	tempdata = (tempdata * 0.01) - 40.0;
if (!bad)
lasttemp = tempdata;


			}
			if (TH == 'H')
			{
				printf("\t\t\t\t\t\t\t\t\tH %s\n", buf);
//				str_temp = Strings.Right(str_data, 12);
// 0123 4567 8901 2345
//      XXXX XXXX XXXX
//				str_msb = Strings.Left(str_temp, 4);
//      XXXX
//				str_lsb = Strings.Right(str_temp, 8);
//           XXXX XXXX

    char str_msb[5];
    char str_lsb[9];

    memcpy(str_msb, buf+4, 4);
    str_msb[4] = '\0';

    memcpy(str_lsb, buf+8, 8);
    str_lsb[8] = '\0';

    double msb = Bin2Dec(str_msb);
    double lsb = Bin2Dec(str_lsb);
   tempdata = (msb * 256.0) + lsb;


            double C1 = -4;
            double C2 = 0.0405;
            double C3 = -2.8E-06;
            double T1 = 0.01;
            double T2 = 0.00008;
            double rh = round(tempdata);
			double rh_lin = ((C3 * rh) * rh) + (C2 * rh) + C1;
            double rh_true = (lasttemp - 25) * (T1 + (T2 * rh)) + rh_lin;
            if (rh_true > 100) { rh_true = 100;}
            if (rh_true < 0.1){ rh_true = 0.1; }
tempdata= rh_true;

			}
/*
			double msb = Bin2Dec(str_msb);
			double lsb = Bin2Dec(str_lsb);
			double tempdata = (msb * 256.0) + lsb;
			double ReadSHT = tempdata;

			Stop_IIC();
*/

temper_switch(0, 1);

if (bad)
return -99;
			return tempdata;
		}
		

		

/*
		private double[] ReadTEMPerHum(){
			decimal ReadTEMPsh10 = new decimal((ReadSHT("T") * 0.01) - 40.0);
			decimal C1 = -4M;
			decimal C2 = 0.0405M;
			object C3 = -2.8E-06;
			decimal T1 = 0.01M;
			decimal T2 = 0.00008M;
			temper_delay(4);
			int rh = (int) Math.Round(ReadSHT("H"));
			decimal rh_lin = Conversions.ToDecimal(Operators.AddObject(Operators.AddObject(Operators.MultiplyObject(Operators.MultiplyObject(C3, rh), rh), decimal.Multiply(C2, new decimal(rh))), C1));
			decimal rh_true = decimal.Add(decimal.Multiply(decimal.Subtract(ReadTEMPsh10, 25M), decimal.Add(T1, decimal.Multiply(T2, new decimal(rh)))), rh_lin);
			if (decimal.Compare(rh_true, 100M) > 0)	{ rh_true = 100M;}
			if (Convert.ToDouble(rh_true) < 0.1){ rh_true = 0.1M; }
			double[] myReturn = {-99, -99};
			myReturn[0] = Convert.ToDouble(ReadTEMPsh10)+GlobalVars.config_calibration_temp;
			myReturn[1] = Convert.ToDouble(rh_true)+GlobalVars.config_calibration_humidity;
			log("TEMP: "+myReturn[0].ToString()+", HUM: "+myReturn[1].ToString());
			return myReturn;
	    }
*/
