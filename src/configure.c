
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NO_OF_REPLACE 9
char *input_string[NO_OF_REPLACE]={"@WUSERHWDR@",   //0
                                   "@WDEPS1@",      //1
                                   "@DEPS2@",       //2
                                   "@USEREXTRA@",   //3
                                   "@DEPS3@",       //4
                                   "@LUSERHWDR@",   //5
                                   "@LUSERHWDEF@",  //6
                                   "@WUSERHWDEF@",  //7
                                   "@WDEPS4@"       //8
                                    };
char *output_string[NO_OF_REPLACE];

void replace(char *s, int *len)
{
int i, j, k;
int m,l1,l2;
for(j=0; j<NO_OF_REPLACE; j++)
  {
  l1=strlen(input_string[j]);
  l2=strlen(output_string[j]);
  m=l1-l2;
  i=0;
nxt_i:;  
  while(i <= len[0] && s[i]!=input_string[j][0])i++;
  k=0;
  while( k+i <= len[0] && 
         s[i+k]==input_string[j][k] && 
         input_string[j][k] != 0)k++;
  if(input_string[j][k] == 0)
    {
    len[0]-=m;
    if(m>0)
      {
      k=i;
      while(k < len[0])
        {
        s[k]=s[k+m];
        k++;
        }
      }
    else
      {
      k=len[0];
      while(k > i)
        {
        k--;
        s[k]=s[k+m];
        }
      }
    k=0;  
    while(k<l2)
      {
      s[i]=output_string[j][k];
      i++;
      k++;
      }
    }
  else
    {
    i++;
    }
  if(i<len[0])goto nxt_i;  
  }  
}

int main(int argc, char *argv[])
{
FILE *file;
char *s;
int i, j;
s=malloc(0x40000);
if(s == NULL)
  {
  printf("\nERROR: could not allocate memory");
  exit(0);
  } 
// Make silly use of parameters to suppress warning from compiler
s[0]=argv[0][0];
i=argc;

// Set up the strings we want to replace the input strings with.
file=fopen("wusers_hwaredriver.c","r");
if(file == NULL)
  {
  output_string[0]="";
  output_string[1]="";
  }
else
  {
  output_string[0]="1";
  output_string[1]="wusers_hwaredriver.c";
  fclose(file);
  }
output_string[2]="";
file=fopen("users_extra.c","r");
if(file == NULL)
  {
  output_string[3]="";
  output_string[4]="";
  }
else
  {
  output_string[3]="1";
  output_string[4]="users_extra.c";
  fclose(file);
  }
output_string[5]="";
output_string[6]="";
file=fopen("wusers_hwaredef.h","r");
if(file == NULL)
  {
  output_string[7]="";
  output_string[8]="";
  }
else
  {
  output_string[7]="1";
  output_string[8]="wusers_hwaredef.h";
  fclose(file);
  }
// ***************************************************
file=fopen("conf.h.in","r");
if(file == NULL)
  {
  printf("Could not open file: conf.h.in\n");
  exit(0);
  }
j=0;
i=1;
while(i != 0 && j< 0x3ffff)
  {  
  i=fread(&s[j],1,1,file);
  j++;
  }
j--;  
printf("\nReading %d bytes from conf.h.in",j);
fclose(file);
replace(s, &j);
file=fopen("conf.h","w");
if(file == NULL)
  {
  printf("\nCould not open conf.h for write");
  exit(0);
  }
i=fwrite(s,j,1,file);
fclose(file);
printf("\n%d bytes written to conf.h",j);
if(i!=1)
  {
  printf("\nERROR Buffer only written in part");
  exit(0);
  }
// ***************************************************
file=fopen("Makefile.in","r");
if(file == NULL)
  {
  printf("\nCould not open file: Makefile.in\n");
  exit(0);
  }
j=0;
i=1;
while(i != 0 && j< 0x3ffff)
  {  
  i=fread(&s[j],1,1,file);
  j++;
  }
j--;  
printf("\nReading %d bytes from Makefile.in",j);
fclose(file);
replace(s, &j);
file=fopen("Makefile","w");
if(file == NULL)
  {
  printf("\nCould not open Makefile for write");
  exit(0);
  }
i=fwrite(s,j,1,file);
fclose(file);
printf("\n%d bytes written to Makefile",j);
if(i!=1)
  {
  printf("\nERROR Buffer only written in part");
  exit(0);
  }
   
return 0;  
}
