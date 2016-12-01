/** \file shapemotion.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */  
#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "buzzer.h"

#define GREEN_LED BIT6
#define SW1 BIT0
#define SW2 BIT1
#define SW3 BIT2
#define SW4 BIT3


static short startTime;
static int score;
static int hit;
static int wall;

static char score1[1];
static u_int sc1;
static char score2[1];
static u_int sc2;

/**Rectangle Layer
 *
 *
 */
typedef struct RectLayer_s {
  AbRect *abRect;
  Vec2 pos, posLast, posNext; /* initially just set pos */
  u_int color;
  struct RectLayer_s *next;
} RectLayer;


AbRect player = {abRectGetBounds, abRectCheck, 13,3};
AbRect smallplayer = {abRectGetBounds, abRectCheck, 7,3};
AbRect ball = {abRectGetBounds, abRectCheck, 7,7};

AbRectOutline fieldOutline = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2-1 , screenHeight/2-1}
};

AbRectOutline lineMiddleField = {	/* playing field */
  abRectOutlineGetBounds, abRectOutlineCheck,   
  { screenWidth/2-1, 1}
};

Layer fieldLineLayer = {		/* playing field as a layer */
  (AbShape *) &lineMiddleField,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
  0
};

Layer fieldLayer = {		/* playing field as a layer */
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLACK,
   &fieldLineLayer
};

Layer player2 = {                //RED PLAYER
  (AbShape *)&player,
  {screenWidth/2, 7},
  {0,0}, {0,0},
  COLOR_RED,
  &fieldLayer
};

Layer player1 = {                //BLUE PLAYER
  (AbShape *)&player,
  {screenWidth/2, screenHeight-7},
  {0,0}, {0,0},
  COLOR_BLUE,
  &player2
};

Layer layer0 = {		/**< Layer with an orange circle */
  (AbShape *)&circle7,
  {(screenWidth/2), (screenHeight/2)}, /**<  */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE,
  &player1,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

typedef struct RectMovLayer_s {
  RectLayer *layer;
  Vec2 velocity;
  struct RectMovLayer_s *next;
} RectMovLayer;

/* initial value of {0,0} will be overwritten */ 
MovLayer ml2 = { &player2, {0,0}, 0};
MovLayer ml1 = { &player1, {0,0}, &ml2}; 

MovLayer ml0 = { &layer0, {4,4}, 0 }; //movelayer for ball

movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
   		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  



void mlAdvanceBall(MovLayer *ml, Region *fence, MovLayer *paddles)
{
  MovLayer *p1 = paddles;
  MovLayer *p2 = paddles->next;
  
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  Region p1Boundary;
  Region p2Boundary;

  abShapeGetBounds(p1->layer->abShape, &p1->layer->pos, &p1Boundary);
  abShapeGetBounds(p2->layer->abShape, &p2->layer->pos, &p2Boundary);

    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis])
	  ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	 startTime = spTimer;
	 wall = 1;
	 if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) & axis == 1)
	  {
	    sc1++;
	    startTime = spTimer;
	    score = 1;
	    //p1->layer->abShape = &smallplayer;
	    //p2->layer->abShape = &player;
	  }
       	if ((shapeBoundary.topLeft.axes[axis] > fence->topLeft.axes[axis]) & axis == 1)
	  {
	    sc2++;
	    startTime = spTimer;
	    score = 1;
	    // p2->layer->abShape = &smallplayer;
	    //p1->layer->abShape = &player;
	  }
	if (sc1 > 8 ){
	  drawString5x7(screenWidth/2-20,screenHeight/2, "P1 WINS", COLOR_BLUE, COLOR_WHITE);
	   or_sr(0x10);
	}
	if (sc2 > 8 ){
	  drawString5x7(screenWidth/2-20,screenHeight/2, "P2 WINS", COLOR_RED, COLOR_WHITE);
	   or_sr(0x10);
	}
    
	score1[0] = '0'+ sc1;
	score2[0] = '0'+ sc2;
  
      }	/**< if outside of fence */

    } /**< for axis */

    if ((shapeBoundary.topLeft.axes[1] < p2Boundary.botRight.axes[1]) &&
	((shapeBoundary.botRight.axes[0] > p2Boundary.topLeft.axes[0]) && (shapeBoundary.topLeft.axes[0] < p2Boundary.botRight.axes[0])))
      {
	int velocityX;
	if ((p2->velocity.axes[0] > 0) & (ml->velocity.axes[0] < 0))
	       velocityX = ml->velocity.axes[0] = -ml->velocity.axes[0];
	if ((p2->velocity.axes[0] < 0) & (ml->velocity.axes[0] > 0))
	       velocityX = ml->velocity.axes[0] = -ml->velocity.axes[0];
	
	int velocityY = ml->velocity.axes[1] = -ml->velocity.axes[1];
	newPos.axes[1] += (2*velocityY);
	startTime = spTimer;
	hit = 1;
      }

     if ((shapeBoundary.botRight.axes[1] > p1Boundary.topLeft.axes[1]) &&
	((shapeBoundary.botRight.axes[0] > p1Boundary.topLeft.axes[0]) && (shapeBoundary.topLeft.axes[0] < p1Boundary.botRight.axes[0])))
      {
	int velocityX;
	if ((p1->velocity.axes[0] > 0) & (ml->velocity.axes[0] < 0))
	       velocityX = ml->velocity.axes[0] = -ml->velocity.axes[0];
	if ((p1->velocity.axes[0] < 0) & (ml->velocity.axes[0] > 0))
	       velocityX = ml->velocity.axes[0] = -ml->velocity.axes[0];
	
	int velocityY = ml->velocity.axes[1] = -ml->velocity.axes[1];
	newPos.axes[1] += (2*velocityY);
	 startTime = spTimer;
	hit = 1;
	}

  ml->layer->posNext = newPos;
   
  // } /**< for ml */
}

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */
void mlAdvance(MovLayer *ml, Region *fence)
{
  
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary;
  
   for (; ml; ml = ml ->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

    for (axis = 0; axis < 2; axis ++) {
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) 
	  ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
       	
    }	/**< if outside of fence */
  } /**< for axis */
  ml->layer->posNext = newPos;
  } /**< for ml */
}

void movePaddle(u_int switches, MovLayer *ml)
{
  MovLayer *p1 = ml;
  MovLayer *p2 = ml->next;

  if (!(switches & SW3)){
      	p1->velocity.axes[0] = -3;
    }
  else if (!(switches & SW4)){
    p1->velocity.axes[0] = 3;
  }
  else{
    p1->velocity.axes[0] = 0;
  }
  
  if (!(switches & SW1)){
      	p2->velocity.axes[0] = -3;
    }
  else if (!(switches & SW2)){
    p2->velocity.axes[0] = 3;
  }
  else{
    p2->velocity.axes[0] = 0;
  }
  }


u_int bgColor = COLOR_WHITE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region arrowSolid;

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */		
  P1OUT |= GREEN_LED;

  spTimer = 0;
  
  configureClocks();
  buzzer_init();
  lcd_init();
  shapeInit();
  p2sw_init(15);

  shapeInit();
 
  layerInit(&layer0);
  layerDraw(&layer0);
  
  layerGetBounds(&fieldLayer, &fieldFence);

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */


  for(;;) {
     u_int switches = p2sw_read(), i;
    movePaddle(switches, &ml1);
    
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    drawString5x7(screenWidth-10,screenHeight/2+4, score1, COLOR_BLUE, COLOR_WHITE);
    drawString5x7(4,screenHeight/2-10, score2, COLOR_RED, COLOR_WHITE);
    //  movLayerDraw(&mlf, &fieldLayer);
    movLayerDraw(&ml0, &layer0);
    movLayerDraw(&ml1, &layer0);  
  }
}
  

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  spTimer ++;
  if (count == 10) {
    mlAdvanceBall(&ml0, &fieldFence, &ml1);
    mlAdvance(&ml1, &fieldFence);
    if (p2sw_read())
      redrawScreen = 1;
    count = 0;
  }

   if(score){
      buzzer_set_period(900);
     if (spTimer >= (startTime + 50)){
       score = 0;
        buzzer_set_period(0); 
     }
   }
     
 if(hit){
   buzzer_set_period(1500);
     if (spTimer >= (startTime + 15)){
       hit = 0;
        buzzer_set_period(0); 
     }
  }
 
 if(wall){
   buzzer_set_period(2200);
     if (spTimer >= (startTime + 15)){
       wall = 0;
        buzzer_set_period(0); 
     }
 }
		    
  
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
