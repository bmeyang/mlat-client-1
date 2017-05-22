#include "stdafx.h"
#include "ac_decoder.h"

ac_decoder::ac_decoder() 
	:check_stat_interval(10)
{
	next_check_time = time(NULL) + check_stat_interval ; 
}

ac_decode_result_t ac_decoder::decode(unsigned char ac[2] )
{
	ac_decode_result_t result ;
	memset(&result ,0 ,sizeof(ac_decode_result_t));

	result.type = AC_MODE_NA ;

	time_t now = time(NULL) ; 
	if(now > next_check_time)
	{
		commit_ac_mode_stat() ; 
		next_check_time = now + check_stat_interval ;
	}

	/*
	AC����ȫΪ0��Ӧ����Ӳ�����ݴ���
	*/
	if(ac[0]+ac[1]==0)
		return result; 

	//Xλ��Ϊ��
	if(ac[1]&0x40)
		return	result; 

	/*
	ACԭʼ���ݸ�ʽΪ:	SPI,0��0��C1�� A1��C2��A2��C4 ,A4��X,B1��D1��B2��D2��B4��D4 , 
	ת�������ݸ�ʽΪ:	00 A4 A2 A1 , 00 B4 B2 B1 , SPI C4 C2 C1 , 00 D4 D2 D1
	*/

	unsigned short modeac =
		((ac[0] & 0x10) ? 0x0010 : 0) |  // C1
		((ac[0] & 0x08) ? 0x1000 : 0) |  // A1
		((ac[0] & 0x04) ? 0x0020 : 0) |  // C2
		((ac[0] & 0x02) ? 0x2000 : 0) |  // A2
		((ac[0] & 0x01) ? 0x0040 : 0) |  // C4
		((ac[1] & 0x80) ? 0x4000 : 0) |  // A4
		((ac[1] & 0x20) ? 0x0100 : 0) |  // B1
		((ac[1] & 0x10) ? 0x0001 : 0) |  // D1
		((ac[1] & 0x08) ? 0x0200 : 0) |  // B2
		((ac[1] & 0x04) ? 0x0002 : 0) |  // D2
		((ac[1] & 0x02) ? 0x0400 : 0) |  // B4
		((ac[1] & 0x01) ? 0x0004 : 0) |  // D4
		((ac[0] & 0x80) ? 0x0080 : 0);   // SPI

	int ac_type = get_ac_type(modeac) ; 
	if(ac_type == AC_MODE_NA)
	{
		return result ; 
	}
	else if(ac_type == AC_MODE_A)
	{
		result.type = AC_MODE_A ;
		result.squawk = modeac & 0x7777 ;
		result.is_spi = modeac & 0x0080?true:false ;
		return result ; 
	}
	else
	{
		result.type = AC_MODE_C ; 
		int alt = modeA2modeC(modeac) ; 
		result.altitude = (alt==AC_INVALID_ALTITUDE)?AC_INVALID_ALTITUDE:(alt*100);
		return	 result ;
	}

}

int ac_decoder::get_ac_type(unsigned short modeac )
{

	/*
	modeac��������˳��:
	00 A4 A2 A1  00 B4 B2 B1  SPI C4 C2 C1  00 D4 D2 D1
	*/
#define MIN_AC_COUNT	3

	/*
	���SPI������֣���ΪAģʽ
	*/
	if(modeac&0x0080)
	{
		return AC_MODE_A ; 
	}

	/*
	a.���squawkΪ������������룬��ΪAģʽ
	*/
	unsigned short	squawk = modeac&0x7777 ;
	if(squawk ==0x7500 ||
		squawk ==0x7600 ||
		squawk == 0x7700)
		return AC_MODE_A ; 

	/*
	b.���C4 C2 C1�Ľ�����C Ϊ0��5��7 ֮һʱ���ж�ΪA ģʽʶ�����
	c.���D4 D2 D1�Ľ�����D Ϊ1��2��3��5��6��7 ֮һʱ���ж�ΪA ģʽʶ�����
	*/
	int cvalue =	(modeac>>4)&0x0007  ; 
	int dvalue =	modeac&0x0007;

	if((cvalue==0 || 
		cvalue==5 || 
		cvalue==7)||
		(dvalue==1 ||
		dvalue==2 || 
		dvalue==3 ||
		dvalue==5 || 
		dvalue==6 || 
		dvalue==7)
		)
	{
		unsigned int counted =get_mode_count_stat(a_mode_stat , modeac);
		inc_mode_stat(a_mode_stat,modeac);
		if(counted ==-1 || counted	>MIN_AC_COUNT)
		{
			//��һ���յ��˱���,���߼���������Ѿ�����յ�����
			return AC_MODE_A ;
		}
		else
		{
			//������Aģʽ���룬�����յ���ͬ����̫�٣��������Ӳ�����մ���
			//������Ϊ���Ǵ���ı���
			return	AC_MODE_NA ;
		}
	}

	//���Ĳ��ܿ϶�ΪAģʽ���ģ�����ΪCģʽ����Aģʽ�����к����ж�
	unsigned int counted =get_mode_count_stat(na_mode_stat ,modeac);
	if(counted >MIN_AC_COUNT)
	{
		/*
		��AC���Ķ���ȶ����֣��������A���룬������Ѳ���߶ȷ��еķɻ�.
		���˱��İ���Cģʽ���н��룬���λ������Ѳ���߶ȣ�5100~14900�ף�(16700~48900 Ӣ��)
		������Ϊ��Cģʽ���룬������Ϊ��A����
		*/

		int altitude = AC_INVALID_ALTITUDE ; 
		int modeC = modeA2modeC(modeac);
		if (modeC != AC_INVALID_ALTITUDE) 
		{
			altitude = modeC * 100;
			if(altitude >=16700 && altitude <=48900)
			{
				//Ŀ��߶������񺽷��и߶�Ҫ����Ϊ��Cģʽ����
				return	 AC_MODE_C ;
			}
			else
			{
				//������ָ���ĸ߶��ڷ��У���Ϊ��Aģʽ����
				inc_mode_stat(a_mode_stat , modeac);
				return AC_MODE_A ;
			}
		}
		else
		{
			//����Cģʽ����ʧ�ܣ�����Ϊ��Aģʽ����
			inc_mode_stat(a_mode_stat , modeac);
			return AC_MODE_A ;
		}

	}

	//��ȫ�޷�ȷ�ϵı���
	inc_mode_stat(na_mode_stat , modeac);
	return AC_MODE_NA; 
}
int ac_decoder::modeA2modeC(unsigned int modea)
{
	unsigned int FiveHundreds = 0;
	unsigned int OneHundreds  = 0;

	if ((modea & 0xFFFF8889) != 0 ||         // check zero bits are zero, D1 set is illegal
		(modea & 0x000000F0) == 0) { // C1,,C4 cannot be Zero
			return AC_INVALID_ALTITUDE;
	}

	if (modea & 0x0010) {OneHundreds ^= 0x007;} // C1
	if (modea & 0x0020) {OneHundreds ^= 0x003;} // C2
	if (modea & 0x0040) {OneHundreds ^= 0x001;} // C4

	// Remove 7s from OneHundreds (Make 7->5, snd 5->7). 
	if ((OneHundreds & 5) == 5) {OneHundreds ^= 2;}

	// Check for invalid codes, only 1 to 5 are valid 
	if (OneHundreds > 5) {
		return AC_INVALID_ALTITUDE;
	}

	//if (ModeA & 0x0001) {FiveHundreds ^= 0x1FF;} // D1 never used for altitude
	if (modea & 0x0002) {FiveHundreds ^= 0x0FF;} // D2
	if (modea & 0x0004) {FiveHundreds ^= 0x07F;} // D4

	if (modea & 0x1000) {FiveHundreds ^= 0x03F;} // A1
	if (modea & 0x2000) {FiveHundreds ^= 0x01F;} // A2
	if (modea & 0x4000) {FiveHundreds ^= 0x00F;} // A4

	if (modea & 0x0100) {FiveHundreds ^= 0x007;} // B1 
	if (modea & 0x0200) {FiveHundreds ^= 0x003;} // B2
	if (modea & 0x0400) {FiveHundreds ^= 0x001;} // B4

	// Correct order of OneHundreds. 
	if (FiveHundreds & 1) {OneHundreds = 6 - OneHundreds;} 

	return ((FiveHundreds * 5) + OneHundreds - 13); 
}

 int ac_decoder::get_mode_count_stat(ac_count_stat_t& which  , unsigned short modea)
{
	ac_count_stat_t::const_iterator it = which.find(modea);
	return (it==which.end())?-1:(it->second.counted) ;
}

void ac_decoder::inc_mode_stat(ac_count_stat_t& which  , unsigned short modea )
{
	//printf("which  = %d\r\n" , which.size());
	ac_count_stat_t::iterator it = which.find(modea); 	
	if(it== which.end()) 	{
		ac_count_stat_item_t item = {0,1 };
		which.insert(std::make_pair(modea	 , item));
	}
	else
	{
		it->second.counting++ ;
	}
}

void ac_decoder::commit_ac_mode_stat()
{
	ac_count_stat_t	::iterator it =a_mode_stat.begin();
	for(; it!=a_mode_stat.end(); ++it)
	{
		it->second.counted =  it->second.counting ;
		it->second.counting = 0 ;
	}

	it =na_mode_stat.begin();
	for(; it!=na_mode_stat.end(); ++it)
	{
		it->second.counted =  it->second.counting ;
		it->second.counting = 0 ;
	}
}

