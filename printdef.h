enum printer_attributes
{
  eLongName,
  eDriver,
  eSupportsColor,
  eModel,
  eParamFunc,
  eMediaSizeFunc,
  eImageableAreaFunc,
  ePrintFunc,
  eBrightness,
  eGamma,
  eContrast,
  eRed,
  eGreen,
  eBlue,
  eSaturation,
  eDensity
};

typedef struct printer
{
  char	driver[64];
  char	name[64];
  int	isColor;
  int	model;
  char	paramfunc[64];
  char	mediasizefunc[64];
  char	imageableareafunc[64];
  char	printfunc[64];
  int	brightness;		/* Output brightness */
  float gamma;                  /* Gamma */
  int   contrast,		/* Output Contrast */
	red,			/* Output red level */
	green,			/* Output green level */
	blue;			/* Output blue level */
  float	saturation;		/* Output saturation */
  float	density;		/* Maximum output density */
} printer_t;

typedef union yylv {
  int ival;
  double dval;
  char *sval;
} YYSTYPE;

extern YYSTYPE yylval;
extern printer_t thePrinter;

#include "printdefy.h"

