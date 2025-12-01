#include <REGX52.H>
void Delay500ms()		//@11.0592MHz
{
	unsigned char i, j, k;

	
	i = 22;
	j = 3;
	k = 227;
	do
	{
		do
		{
			while (--k);
		} while (--j);
	} while (--i);
}
 void main()
 {
	 P2=0xFF;
	 Delay500ms();
	 p2=0xFE;
	 Delay500ms();
 }