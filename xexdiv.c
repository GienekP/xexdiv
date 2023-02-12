/*--------------------------------------------------------------------*/
/* XEX Divider - GienekP (c)2023                                      */
/*--------------------------------------------------------------------*/
#include <stdio.h>
/*--------------------------------------------------------------------*/
typedef unsigned char U8;
/*--------------------------------------------------------------------*/
#define MAXPOS 9
/*--------------------------------------------------------------------*/
void xexdivider(const char *fn, const char *name)
{
	const unsigned int limit=(8192-32-6);
	const char car[]="CAR:";
	const char load[]="LOAD /L A ";	
	char mainfn[]="_______0.BAT";
	char partfn[]="_______0.EXE";
	U8 xex[65536];
	unsigned int start[MAXPOS],end[MAXPOS],length[MAXPOS],ptr[MAXPOS];
	char type[MAXPOS];
	U8 bank[MAXPOS];
	unsigned int i,j,size=0,nob=0,nop=0,f=0,sum=0;
	FILE *pf;
	
	for (i=0; i<sizeof(xex); i++) {xex[i]=0;};
	for (i=0; i<MAXPOS; i++)
	{
		start[i]=0;
		end[i]=0;
		length[i]=0;
		ptr[i]=0;
		type[i]=' ';
		bank[i]=0;
	};
	
	// Prepare NAME
	for (i=0; i<7; i++)
	{
		char c=name[i];
		if (c) 
		{
			mainfn[i]=c;
			partfn[i]=c;
		}
		else {i=8;};
	};
	
	// Read XEX
    pf=fopen(fn,"rb");
    if (pf)
    {
		fseek(pf,0,SEEK_END);
		size=ftell(pf);
		fseek(pf,0,0);
		fread(xex,sizeof(U8),size,pf);
		fclose(pf);
	};
	
	// Test FFFF
	if ((xex[0]==0xFF) && (xex[1]==0xFF)) {printf("Binary DOS File %i bytes\n",size);}
	else {size=0;};
	
	// Assign blocks
	for (i=0; i<size; i++)
	{
		if ((xex[i]==0xFF) && (xex[i+1]==0xFF))
		{
			unsigned int s,e;
			s=xex[i+3];
			s<<=8;
			s|=xex[i+2];
			e=xex[i+5];
			e<<=8;
			e|=xex[i+4];
			start[nob]=s;
			end[nob]=e;
			length[nob]=(e-s)+1;
			i+=6;
			ptr[nob]=i;
			i+=(e-s);
			nob++;
		}
		else
		{
			unsigned int s,e;
			s=xex[i+1];
			s<<=8;
			s|=xex[i+0];
			e=xex[i+3];
			e<<=8;
			e|=xex[i+2];
			start[nob]=s;
			end[nob]=e;
			length[nob]=(e-s)+1;
			i+=4;
			ptr[nob]=i;
			i+=(e-s);	
			nob++;		
		};
	};
	
	// Detect type
	for (i=0; i<nob; i++)
	{
		if ((start[i]==0x02E0) && (end[i]==0x02E1)) {type[i]='R';}
		else if ((start[i]==0x02E2) && (end[i]==0x02E3)) {type[i]='I';}
		else {type[i]='D';}
		if (length[i]>limit) {f=1;};
	};

	// Show readed blocks
	printf("Readed blocks:\n");
	for (i=0; i<nob; i++)
	{
		printf("Block %c: $%04X - $%04X ( %i B )  Ptr: 0x%04X\n",type[i],start[i],end[i],length[i],ptr[i]);
	};

	// Divide blocks
	while (f)
	{
		f=0;
		for (i=0; i<nob; i++)
		{
			if ((length[i]>limit) && (nob<MAXPOS))
			{
				unsigned int j;
				for (j=(nob-1); j>i; j--)
				{
					start[j+1]=start[j];
					end[j+1]=end[j];
					length[j+1]=length[j];
					ptr[j+1]=ptr[j];
					type[j+1]=type[j];
				};
				nob++;
				end[i+1]=end[i];
				end[i]=start[i]+limit-1;
				start[i+1]=end[i]+1;
				length[i+1]=length[i]-limit;
				length[i]=limit;
				ptr[i+1]=ptr[i]+limit;
				f=1;
				i=nob;
			};
		};
	};	

	// Assign bank
	for (i=0; i<nob; i++)
	{
		bank[i]=nop;
		sum+=length[i];
		if (sum>=limit)
		{
			sum=0;
			nop++;
		};
	};
	
	// Show sorted blocks
	printf("Final blocks:\n");
	for (i=0; i<nob; i++)
	{
		printf("Bank %i:  Block %c: $%04X - $%04X ( %i B )  Ptr: 0x%04X\n",bank[i],type[i],start[i],end[i],length[i],ptr[i]);
	};
	
	printf("Generated:\nAUTOEXEC.BAT\n%s\n",mainfn);
	
	// Save files for banks
	for (i=0; i<=nop; i++)
	{
		partfn[sizeof(partfn)-6]=('1'+i);
		pf=fopen(partfn,"wb");
		if (pf)
		{
			printf("%s\n",partfn);
			fputc(0xFF,pf);
			fputc(0xFF,pf);
			for (j=0; j<nob; j++)
			{
				if (bank[j]==i)
				{
					fputc(start[j]&0xFF,pf);
					fputc((start[j]>>8)&0xFF,pf);
					fputc(end[j]&0xFF,pf);
					fputc((end[j]>>8)&0xFF,pf);
					fwrite(&xex[ptr[j]],sizeof(U8),length[j],pf);
				};
			};
			fclose(pf);		
		};
	};	
	
	// Save NAME.BAT
    pf=fopen(mainfn,"wb");
    if (pf)
    {
		for (i=0; i<=nop; i++)
		{
			partfn[sizeof(partfn)-6]=('1'+i);
			fwrite(load,sizeof(char),sizeof(load)-1,pf);
			fwrite(car,sizeof(char),sizeof(car)-1,pf);
			fwrite(partfn,sizeof(char),sizeof(partfn)-1,pf);
			fputc(0x9B,pf);
		};
		fputc(0x9B,pf);
		fclose(pf);	
	};	
	
	// Save AUTOEXEC.BAT
    pf=fopen("AUTOEXEC.BAT","wb");
    if (pf)
    {
		fputc('-',pf);
		fwrite(car,sizeof(char),sizeof(car)-1,pf);
		fwrite(mainfn,sizeof(char),sizeof(mainfn)-1,pf);		
		fputc(0x9B,pf);
		fputc(0x9B,pf);
		fclose(pf);	
	};
}
/*--------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
	printf("XEX divider\n");
	switch (argc)
	{
		case 3:
		{
			xexdivider(argv[1],argv[2]);
		} break;		
		default:
		{
			printf("use:\n");
			printf("   xexdiv file.xex\n");	
			printf("   xexdiv file.xex NAME\n");	
		} break;
	};
	return 0;
}
/*--------------------------------------------------------------------*/
