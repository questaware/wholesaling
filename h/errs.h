
/* Errors returned by the database fall into 3 categories:
	1) Application Induced errors
	2) Minor Internal Errors
	3) Critical Internal Errors (Failures)
*/

#define ENOTTHERE (-1)
#define ELOGIC    (-2)


#define AIE_BASE 0

#define  ANY_ERROR      -(AIE_BASE+1)  /* -1 */
#define  DB_EXISTS      -(AIE_BASE+2)  /* -2 */
#define  DUP_UK_ATTR    -(AIE_BASE+3)  /* -3 */
#define  TOO_MANY_DBS   -(AIE_BASE+4)  /* -4 */
#define  OUT_OF_ROOT    -(AIE_BASE+5)  /* -5 */
#define  CLASS_EXISTS   -(AIE_BASE+6)  /* -6 */
#define  IX_EXISTS      -(AIE_BASE+7)  /* -7 */
#define  ENY_EXISTS     -(AIE_BASE+8)  /* -8 */
#define  ENV_FAULT      -(AIE_BASE+9)  /* -9 */
#define  TBL_FULL       -(AIE_BASE+10) /* -10*/
#define  NOT_PRESENT    -(AIE_BASE+11) /* -11*/
#define  OLD_BAD_BUTTON -(AIE_BASE+12) /* -12*/  // not in use
#define  TOO_BIG        -(AIE_BASE+13) /* -13*/
#define  INVALID        -(AIE_BASE+14) /* -14*/
#define  ILLEGAL_SRCH   -(AIE_BASE+15) /* -15*/
#define  IN_USE         -(AIE_BASE+16) /* -16*/
#define  CURR_UNDEFINED -(AIE_BASE+17) /* -17 */
#define  REC_GONE       -(AIE_BASE+18) /* -18 */


#define MIE_BASE   100

#define MISSING_BLOCK  -(MIE_BASE+1)   /* -101 */
#define PI_TOO_DEEP    -(MIE_BASE+2)   /* -102 */
#define PI_MISSING     -(MIE_BASE+3)   /* -103 */
#define PI_FAIL        -(MIE_BASE+4)   /* -104 */
#define CORRUPT_PF     -(MIE_BASE+5)   /* -105 */
#define KEY_MISSING    -(MIE_BASE+6)   /* -106 */
#define FILE_FAIL      -(MIE_BASE+7)   /* -107 */
#define BAD_KEY	       -(MIE_BASE+8)   /* -108 */
#define DISK_SPACE     -(MIE_BASE+9)   /* -109 */
#define OUT_OF_MEMORY  -(MIE_BASE+10)  /* -110 */
#define FAULT          -(MIE_BASE+11)  /* -111 */


#define CIE_BASE   200

#define LOST_BLOCK     -(CIE_BASE+1)  /* -201 */
#define CORRUPT_DB     -(CIE_BASE+2)  /* -202 */
#define PRECOND        -(CIE_BASE+3)  /* -203 */
	
	
