#include "SH1106Ui.h"


SH1106Ui::SH1106Ui(SH1106 *display) {
  this->display = display;
}

void SH1106Ui::init() {
  this->display->init();
}

void SH1106Ui::setTargetFPS(byte fps){
  int oldInterval = this->updateInterval;
  this->updateInterval = ((float) 1.0 / (float) fps) * 1000;

  // Calculate new ticksPerFrame
  float changeRatio = oldInterval / this->updateInterval;
  this->ticksPerFrame *= changeRatio;
  this->ticksPerTransition *= changeRatio;
}

// -/------ Automatic controll ------\-

void SH1106Ui::enableAutoTransition(){
  this->autoTransition = true;
}
void SH1106Ui::disableAutoTransition(){
  this->autoTransition = false;
}
void SH1106Ui::setAutoTransitionForwards(){
  this->frameTransitionDirection = 1;
}
void SH1106Ui::setAutoTransitionBackwards(){
  this->frameTransitionDirection = 1;
}
void SH1106Ui::setTimePerFrame(int time){
  this->ticksPerFrame = (int) ( (float) time / (float) updateInterval);
}
void SH1106Ui::setTimePerTransition(int time){
  this->ticksPerTransition = (int) ( (float) time / (float) updateInterval);
}


// -/------ Customize indicator position and style -------\-
void SH1106Ui::setIndicatorPosition(IndicatorPosition pos) {
  this->indicatorPosition = pos;
  this->dirty = true;
}
void SH1106Ui::setIndicatorDirection(IndicatorDirection dir) {
  this->indicatorDirection = dir;
}
void SH1106Ui::setActiveSymbole(const char* symbole) {
  this->activeSymbole = symbole;
  this->dirty = true;
}
void SH1106Ui::setInactiveSymbole(const char* symbole) {
  this->inactiveSymbole = symbole;
  this->dirty = true;
}


// -/----- Frame settings -----\-
void SH1106Ui::setFrameAnimation(AnimationDirection dir) {
  this->frameAnimationDirection = dir;
}
void SH1106Ui::setFrames(FrameCallback* frameFunctions, int frameCount) {
  this->frameCount     = frameCount;
  this->frameFunctions = frameFunctions;
}

// -/----- Overlays ------\-
void SH1106Ui::setOverlays(OverlayCallback* overlayFunctions, int overlayCount){
  this->overlayCount     = overlayCount;
  this->overlayFunctions = overlayFunctions;
}


// -/----- Manuel control -----\-
void SH1106Ui::nextFrame() {
  this->state.frameState = IN_TRANSITION;
  this->state.ticksSinceLastStateSwitch = 0;
  this->frameTransitionDirection = 1;
}
void SH1106Ui::previousFrame() {
  this->state.frameState = IN_TRANSITION;
  this->state.ticksSinceLastStateSwitch = 0;
  this->frameTransitionDirection = -1;
}


// -/----- State information -----\-
SH1106UiState SH1106Ui::getUiState(){
  return this->state;
}


int SH1106Ui::update(){
  int timeBudget = this->updateInterval - (millis() - this->state.lastUpdate);
  if ( timeBudget <= 0) {
    // Implement frame skipping to ensure time budget is keept
    if (this->autoTransition && this->state.lastUpdate != 0) this->state.ticksSinceLastStateSwitch += ceil(-timeBudget / this->updateInterval);

    this->state.lastUpdate = millis();
    this->tick();
  }
  return timeBudget;
}


void SH1106Ui::tick() {
  this->state.ticksSinceLastStateSwitch++;

  switch (this->state.frameState) {
    case IN_TRANSITION:
        this->dirty = true;
        if (this->state.ticksSinceLastStateSwitch >= this->ticksPerTransition){
          this->state.frameState = FIXED;
          this->state.currentFrame = getNextFrameNumber();
          this->state.ticksSinceLastStateSwitch = 0;
        }
      break;
    case FIXED:
      if (this->state.ticksSinceLastStateSwitch >= this->ticksPerFrame){
          if (this->autoTransition){
            this->state.frameState = IN_TRANSITION;
            this->dirty = true;
          }
          this->state.ticksSinceLastStateSwitch = 0;
      }
      break;
  }

  if (this->dirty) {
    this->dirty = false;
    this->display->clear();
    this->drawIndicator();
    this->drawFrame();
    this->drawOverlays();
    this->display->display();
  }
}

void SH1106Ui::drawFrame(){
  switch (this->state.frameState){
     case IN_TRANSITION: {
       float progress = (float) this->state.ticksSinceLastStateSwitch / (float) this->ticksPerTransition;
       int x, y, x1, y1;
       switch(this->frameAnimationDirection){
        case SLIDE_LEFT:
          x = -128 * progress;
          y = 0;
          x1 = x + 128;
          y1 = 0;
          break;
        case SLIDE_RIGHT:
          x = 128 * progress;
          y = 0;
          x1 = x - 128;
          y1 = 0;
          break;
        case SLIDE_UP:
          x = 0;
          y = -64 * progress;
          x1 = 0;
          y1 = y + 64;
          break;
        case SLIDE_DOWN:
          x = 0;
          y = 64 * progress;
          x1 = 0;
          y1 = y - 64;
          break;
       }

       // Invert animation if direction is reversed.
       int dir = frameTransitionDirection >= 0 ? 1 : -1;
       x *= dir; y *= dir; x1 *= dir; y1 *= dir;

       this->dirty |= (this->frameFunctions[this->state.currentFrame])(this->display, &this->state, x, y);
       this->dirty |= (this->frameFunctions[this->getNextFrameNumber()])(this->display, &this->state, x1, y1);
       break;
     }
     case FIXED:
      this->dirty |= (this->frameFunctions[this->state.currentFrame])(this->display, &this->state, 0, 0);
      break;
  }
}

void SH1106Ui::drawIndicator() {
    byte posOfCurrentFrame;

    switch (this->indicatorDirection){
      case LEFT_RIGHT:
        posOfCurrentFrame = this->state.currentFrame;
        break;
      case RIGHT_LEFT:
        posOfCurrentFrame = (this->frameCount - 1) - this->state.currentFrame;
        break;
    }

    for (byte i = 0; i < this->frameCount; i++) {

      const char *image;

      if (posOfCurrentFrame == i) {
         image = this->activeSymbole;
      } else {
         image = this->inactiveSymbole;
      }

      int x,y;
      switch (this->indicatorPosition){
        case TOP:
          y = 0;
          x = 64 - (12 * frameCount / 2) + 12 * i;
          break;
        case BOTTOM:
          y = 56;
          x = 64 - (12 * frameCount / 2) + 12 * i;
          break;
        case RIGHT:
          x = 120;
          y = 32 - (12 * frameCount / 2) + 12 * i;
          break;
        case LEFT:
          x = 0;
          y = 32 - (12 * frameCount / 2) + 12 * i;
          break;
      }

      this->display->drawXbm(x, y, 8, 8, image);
    }
}

void SH1106Ui::drawOverlays() {
 for (int i=0;i<this->overlayCount;i++){
    this->dirty |= (this->overlayFunctions[i])(this->display, &this->state);
 }
}

int SH1106Ui::getNextFrameNumber(){
  int nextFrame = (this->state.currentFrame + this->frameTransitionDirection) % this->frameCount;
  if (nextFrame < 0){
    nextFrame = this->frameCount + nextFrame;
  }
  return nextFrame;
}
