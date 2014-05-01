//
//  complex.cpp
//  RLC Meter
//
//  Created by Balmer on 13.04.14.
//
//

#include "hw_config.h"
#include <math.h>
#include <stdbool.h>
#include "voltage.h"
#include "calc_rc.h"
#include "mcp6s21.h"
#include "lcd_interface.h"
#include "corrector.h"
#include "dac.h"

complexf lastZx = 0;
bool lastZxFilled = false;

void OnCalculate()
{
	float gain_V = getGainValueV();
	float gain_I = getGainValueI();
	float resistor = getResistorOm();
	float toVolts = 3.3f/4095.0f;
	complexf zV = g_data.ch_v.k_sin + g_data.ch_v.k_cos*I;
	complexf zI = g_data.ch_i.k_sin + g_data.ch_i.k_cos*I;
	zV *= toVolts/gain_V;
	zI *= toVolts/gain_I;

	complexf Zx = (zV/zI)*resistor;

	lastZxFilled = true;
	calculatedValues = true;
	lastZx = Zx;

	Zx = Corrector(Zx);

	Rre = creal(Zx);
	Rim = cimag(Zx);

	float F = DacFrequency();

	if(isSerial)
	{
		valueIsC = false;
		valueL = cimag(Zx)/(2*pi*F);

		if(cimag(Zx)<-1e-10f)
		{
			valueIsC = true;
			valueC = -1/(2*pi*F*cimag(Zx));
		} else
		{
			valueC = 0;
		}
		//если сопротивление маленькое и индуктивность немного отрицательная, то таки считаем что это ошибка калибрации
		if(cabs(Zx)<1 && valueL<0 && valueL>-20e-9)
			valueIsC = false;
	} else
	{//parrallel
		valueIsC = true;
		complexf Yx = 1/Zx;
		valueC = cimag(Yx)/(2*pi*F);

		if(cimag(Yx)<-1e-10)
		{
			valueIsC = false;
			valueL = -1/(2*pi*F*cimag(Yx));
		} else
		{
			valueL = 0;
		}

		//если сопротивление большое и емкость немного отрицательная, то таки считаем что это ошибка калибрации
		if(cabs(Zx)>1e5f && valueC<0 && valueC>-5e-12)
			valueIsC = true;

		Rre = 1.0f/creal(Yx);
		Rim = 1.0f/cimag(Yx);
	}

}