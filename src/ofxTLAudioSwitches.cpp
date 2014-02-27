/**
 * ofxTimeline
 * openFrameworks graphical timeline addon
 *
 * Copyright (c) 2011-2012 James George
 * Development Supported by YCAM InterLab http://interlab.ycam.jp/en/
 * http://jamesgeorge.org + http://flightphase.com
 * http://github.com/obviousjim + http://github.com/flightphase
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "ofxTLAudioSwitches.h"
#include "ofxTimeline.h"
#include "ofxHotKeys.h"

ofxTLAudioSwitches::ofxTLAudioSwitches(){
	placingSwitch = NULL;
    lastTimelinePoint = 0;
    enteringText = false;
	clickedTextField = NULL;

	lastFFTPosition = -1;
	defaultSpectrumBandwidth = 1024;
    trackIsPlaying = 0;

    playOnUpdate = false;
    stopOnUpdate = false;

}

ofxTLAudioSwitches::~ofxTLAudioSwitches(){
    
}

bool ofxTLAudioSwitches::loadSoundfile(string filepath){
    ofxTLAudioSwitch* switchKey = new ofxTLAudioSwitch();
    switchKey->time = 0;
	switchKey->soundLoaded = false;
    
	if(switchKey->player.loadSound(filepath, false)){
    	switchKey->soundLoaded = true;
		switchKey->soundFilePath = filepath;
        switchKey->player.getSpectrum(defaultSpectrumBandwidth);
        switchKey->player.setLogAverages(88, 20); //magic numbers defaults from audioTrack
        switchKey->shouldRecomputePreview = true;
        switchKey->timeRange = ofLongRange( 0, switchKey->player.getDuration() *1000 );
        keyframes.push_back(switchKey);
		updateKeyframeSort();
    }
	return switchKey->soundLoaded;
}

bool ofxTLAudioSwitches::isSoundLoaded(){
	return false;
}

void ofxTLAudioSwitches::update(){
    long thisTimelinePoint = currentTrackTime();
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        
        if( timeline->getInOutRangeMillis().intersects(switchKey->timeRange) ){
            if( switchKey->timeRange.contains( lastTimelinePoint ) != 
                switchKey->timeRange.contains( thisTimelinePoint ) ) {
                switchStateChanged(keyframes[i]);
                if( getIsPlaying() ){
                    if( switchKey->timeRange.contains( thisTimelinePoint ) ){
                        if( switchKey->soundLoaded ){
                           switchKey->player.setPositionMS( positionFromMillis(switchKey, currentTrackTime() ) );
                        }
                        if( !switchKey->player.getIsPlaying() ) {
                           switchKey->player.play();
                        }
                    }
                    else if( switchKey->timeRange.contains( lastTimelinePoint ) ) {
                        if( switchKey->soundLoaded && switchKey->player.getIsPlaying() ){
                           switchKey->player.stop();
                        }
                    }
                }
            }
        }
    }
    /*
    //crude.  Need some better way of keeping a skip from one switch to another
    //from double playing.
    if( playOnUpdate && stopOnUpdate ){
        playOnUpdate = false;
        stopOnUpdate = false;
    }
    else if( playOnUpdate ) {
        if( isSoundLoaded() ){
            player.setPositionMS( positionFromMillis(currentTrackTime() ) );
        }
        if(!player.getIsPlaying() ){
            player.play(); 
        }
        playOnUpdate = false;
    }
    else if( stopOnUpdate ){
        if( player.getIsPlaying() && isSoundLoaded() ){
            player.stop();
        }
        stopOnUpdate = false;
    }
    */
    lastTimelinePoint = thisTimelinePoint;
}

void ofxTLAudioSwitches::switchStateChanged(ofxTLKeyframe* key){
    ofxTLSwitchEventArgs args;
    args.sender = timeline;
    args.track = this;
    args.on = isOn();
    args.switchName = ((ofxTLAudioSwitch*)key)->textField.text;
    ofNotifyEvent(events().switched, args);


}

bool ofxTLAudioSwitches::getIsPlaying(){
//return whether the track, _not_ the player, is playing
   return trackIsPlaying;
}

void ofxTLAudioSwitches::draw(){
    
    ofPushStyle();
	ofFill();
	
	//draw a little wobble if its on
	//if(isOnAtMillis(timeline->getCurrentTimeMillis())){
	//play solo change
	if(isOn()){
		ofSetColor(timeline->getColors().disabledColor, 20+(1-powf(sin(ofGetElapsedTimef()*5)*.5+.5,2))*20);
		ofRect(bounds);
	}

    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        float startScreenX = MAX(millisToScreenX(switchKey->timeRange.min), 0);
        float endScreenX = MIN(millisToScreenX(switchKey->timeRange.max), bounds.getMaxX());
		if(startScreenX >= endScreenX){
			continue;
		}

		switchKey->display = ofRectangle(startScreenX, bounds.y, endScreenX-startScreenX, bounds.height);

        //draw handles

        ofSetLineWidth(2);
        bool keyIsSelected = isKeyframeSelected(switchKey);
        if(keyIsSelected || switchKey->startSelected){
	        ofSetColor(timeline->getColors().textColor);
        }
        else{
	        ofSetColor(timeline->getColors().keyColor);    
        }

        ofLine(switchKey->display.x, bounds.y, 
               switchKey->display.x, bounds.y+bounds.height);

        if(keyIsSelected || switchKey->endSelected){
	        ofSetColor(timeline->getColors().textColor);                
        }
        else{
	        ofSetColor(timeline->getColors().keyColor);    
        }        
        ofLine(switchKey->display.x+switchKey->display.width, bounds.y, 
               switchKey->display.x+switchKey->display.width, bounds.y+bounds.height);

        //draw region
        if(keyIsSelected){
        	ofSetColor(timeline->getColors().textColor, 100);    
        }
        else{
        	ofSetColor(timeline->getColors().keyColor, 100);
        }
        //set overlay colors, this will override the colors above
        if(hoverKeyframe == switchKey){
            if(startHover){
                ofPushStyle();
                if(switchKey->startSelected){
                    ofSetColor(timeline->getColors().highlightColor);
                }
                else{
                    ofSetColor(timeline->getColors().keyColor);
                }
                ofRect(switchKey->display.x-2, bounds.y, 4, bounds.height);
                ofPopStyle();
            }
            else if(endHover){
				ofPushStyle();
                if(switchKey->endSelected){
                    ofSetColor(timeline->getColors().highlightColor);
                }
                else{
                    ofSetColor(timeline->getColors().keyColor);
                }
                ofRect(switchKey->display.x+switchKey->display.width-2, bounds.y, 4.0, bounds.height);
                ofPopStyle();
            }
            else {
                if(keyIsSelected){
	                ofSetColor(timeline->getColors().highlightColor);                    
                }else {
	                ofSetColor(timeline->getColors().keyColor);    
                }
            }
        }
        ofRect(switchKey->display);

        if( switchKey->soundLoaded ){
           ofSetColor(ofColor( 50, 50, 50, 50 ));
           if(switchKey->shouldRecomputePreview || viewIsDirty){
               //TODO: XXX: Dear lord, fix this monstrosity!!
               ofRange posVisRange = ofFloatRange( 
                               ofMap( startScreenX, 
                                      millisToScreenX(switchKey->timeRange.min),
                                      millisToScreenX(switchKey->timeRange.min + switchKey->player.getDuration() * 1000),
                                      0.0, 1.0 ),
                               ofMap( endScreenX, 
                                      millisToScreenX(switchKey->timeRange.min),
                                      millisToScreenX(switchKey->timeRange.min + switchKey->player.getDuration() * 1000),
                                      0.0, 1.0 ) );
               recomputePreview(switchKey, endScreenX - startScreenX, posVisRange);
           }

           //draw preview
           for(int i = 0; i < switchKey->previews.size(); i++){
               ofPushMatrix();
               ofTranslate( startScreenX, 0, 0);
               ofScale(computedZoomBounds.span()/zoomBounds.span(), 1, 1);
               switchKey->previews[i].draw();
               ofPopMatrix();
           }
       }
    }
    
    ofPopStyle();
}

bool ofxTLAudioSwitches::isOnAtMillis(long millis){
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        if(switchKey->timeRange.min > millis){
            break;
        }
        if(switchKey->timeRange.contains(millis)){
            return true;
        }
    }
    return false;    
}

bool ofxTLAudioSwitches::isOn(){
	return isOnAtMillis(currentTrackTime());
}

bool ofxTLAudioSwitches::isOnAtPercent(float percent){
    unsigned long long millis = percent*timeline->getDurationInMilliseconds();
    return isOnAtMillis(millis);
}

ofxTLAudioSwitch* ofxTLAudioSwitches::getActiveSwitchAtMillis(long millis){
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        if(switchKey->timeRange.min > millis){
            break;
        }
        if(switchKey->timeRange.contains(millis)){
            return switchKey;
        }
    }
    return NULL;
}

float ofxTLAudioSwitches::positionFromMillis( ofxTLAudioSwitch* switchKey, long millis ){
//returns the player position in millis within the current switch, -1 if not in switch
    if( switchKey != NULL ){
        return millis - switchKey -> timeRange.min;
    }
    else{
        return -1.0;
    }
}

void ofxTLAudioSwitches::recomputePreview( ofxTLAudioSwitch* audioSwitch, int width, ofFloatRange posVisRange){
	
	audioSwitch->previews.clear();
	
//	cout << "recomputing view with zoom bounds of " << zoomBounds << endl;
	
	float trackHeight = bounds.height/(1+audioSwitch->player.getNumChannels());
	int numChannels = audioSwitch->player.getNumChannels();
	vector<short> & buffer  = audioSwitch->player.getBuffer();
	int numSamples = buffer.size() / numChannels;

	for(int c = 0; c < numChannels; c++){
		ofPolyline preview;
		int lastFrameIndex = 0;
		preview.resize(width*2);  //Why * 2? Because there are two points per pixel, center and outside. 
		for(int i = 0; i < width; i++){
            float pointInTrack = ofMap( i, 0, width, posVisRange.min, posVisRange.max );
			float trackCenter = bounds.y + trackHeight * (c+1);
			
			ofPoint * vertex = & preview.getVertices()[ i * 2 ];
			
			if(pointInTrack >= 0 && pointInTrack <= 1.0){
				//draw sample at pointInTrack * waveDuration;
				int frameIndex = pointInTrack * numSamples;					
				float losample = 0;
				float hisample = 0;
				for(int f = lastFrameIndex; f < frameIndex; f++){
					int sampleIndex = f * numChannels + c;
					float subpixelSample = buffer[sampleIndex]/32565.0;
					if(subpixelSample < losample) {
						losample = subpixelSample;
					}
					if(subpixelSample > hisample) {
						hisample = subpixelSample;
					}
				}
				
				if(losample == 0 && hisample == 0){
					//preview.addVertex(i, trackCenter);
					vertex->x = i;
					vertex->y = trackCenter;
					vertex++;
				}
				else {
					if(losample != 0){
//						preview.addVertex(i, trackCenter - losample * trackHeight);
						vertex->x = i;
						vertex->y = trackCenter - losample * trackHeight*.5;
						vertex++;
					}
					if(hisample != 0){
						//ofVertex(i, trackCenter - hisample * trackHeight);
//						preview.addVertex(i, trackCenter - hisample * trackHeight);
						vertex->x = i;
						vertex->y = trackCenter - hisample * trackHeight*.5;
						vertex++;
					}
				}
				
				while (vertex < & preview.getVertices()[ i * 2 ] + 2) {
					*vertex = *(vertex-1);
					vertex++;
				}

				lastFrameIndex = frameIndex;
			}
			else{
				*vertex++ = ofPoint(i,trackCenter);
				*vertex++ = ofPoint(i,trackCenter);
			}
		}
		preview.simplify();
		audioSwitch->previews.push_back(preview);
	}
	computedZoomBounds = zoomBounds;
	audioSwitch->shouldRecomputePreview = false;
}


void ofxTLAudioSwitches::playbackStarted(ofxTLPlaybackEventArgs& args){
	ofxTLTrack::playbackStarted(args);
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        if( switchKey->timeRange.contains( currentTrackTime() ) &&
                switchKey->soundLoaded ) {
            switchKey->player.setPositionMS(
                positionFromMillis(switchKey,currentTrackTime() ) );
            switchKey->player.play();
        }
    }

    trackIsPlaying = true;
}

void ofxTLAudioSwitches::playbackEnded(ofxTLPlaybackEventArgs& args){
    stopOnUpdate = true;
    trackIsPlaying = false;
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        switchKey->player.stop();
    }
}

bool ofxTLAudioSwitches::mousePressed(ofMouseEventArgs& args, long millis){
    
    clickedTextField = NULL;
    //look at each element to see if a text field was clicked
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
		if(switchKey->textFieldDisplay.inside(args.x, args.y)){
            clickedTextField = switchKey;
            break;
        }
    }
    
    //if so, select that text field and key and present modally
    //so that keyboard input all goes to the text field.
    //selection model is designed so that you can type into
    //mulitple fields at once
    if(clickedTextField != NULL){
        timeline->presentedModalContent(this);
        if(!ofGetModifierSelection()){
            timeline->unselectAll();
        }
		if(ofGetModifierSelection() && clickedTextField->textField.getIsEditing()){
			clickedTextField->textField.endEditing();
		}
		else{
			clickedTextField->textField.beginEditing();
			enteringText = true;
			//make sure this key is selected
			selectKeyframe(clickedTextField);
		}
        return false;
    }
	else{
		if(enteringText && !isHovering()){
			for(int i = 0; i < selectedKeyframes.size(); i++){
				((ofxTLAudioSwitch*)selectedKeyframes[i])->textField.endEditing();
			}
			enteringText = false;
			timeline->dismissedModalContent();
		}
	}
	
	if(enteringText)
        return false;
    
    // ---
    if(placingSwitch != NULL){
		if(isActive() && args.button == 0){
			placingSwitch->timeRange.max = millis;
			updateTimeRanges();
		}
		else {
			deleteKeyframe(placingSwitch);
		}
		placingSwitch = NULL;
		return false;
	}
	
	keysAreDraggable = !ofGetModifierSelection();
	
    //check to see if we are close to any edges, if so select them
    bool startSelected = false;
    bool endSelected = false;
    int selectedKeyframeIndex;
    if(isActive() && args.button == 0){
        for(int i = 0; i < keyframes.size(); i++){
            
            ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
            //unselect everything else if we just clicked this edge without shift held down
            startSelected = abs(switchKey->display.x - args.x) < 10.0;
            if (startSelected && !switchKey->startSelected && !ofGetModifierSelection()) {
                timeline->unselectAll();
            }
            //Deselect the key if we clicked it already selected with shift held down
            if(ofGetModifierSelection() && ((startSelected && switchKey->startSelected) || isKeyframeSelected(switchKey))){
                switchKey->startSelected = false;    
            }
            else {
                switchKey->startSelected |= startSelected;
            }
            float endEdge = switchKey->display.x+switchKey->display.width;
            endSelected = abs(endEdge - args.x) < 10.0;
            //don't let them both be selected in one click!
            if(!startSelected && endSelected && !switchKey->endSelected && !ofGetModifierSelection()){
                timeline->unselectAll();
            }
            //Deselect the key if we clicked it already selected with shift held down
            if(ofGetModifierSelection() && ((endSelected && switchKey->endSelected) || isKeyframeSelected(switchKey))){
                switchKey->endSelected = false;    
            }
            else{
                switchKey->endSelected |= endSelected && !startSelected;
            }
            
            if(startSelected || endSelected){
				selectedKeyframeIndex = i;
                break;
            }        
        }
    }
    
    //update dragging and snapping if we clicked an edge
    updateEdgeDragOffsets(millis);
    if(endSelected || startSelected){
        ofxTLAudioSwitch* selectedSwitch = (ofxTLAudioSwitch*)keyframes[selectedKeyframeIndex];
        timeline->setDragTimeOffset(selectedSwitch->edgeDragOffset);
    }
	
    if(!endSelected && !startSelected){
    	//normal selection from above

	    ofxTLKeyframes::mousePressed(args, millis);
        if(isActive()){
	        timeline->cancelSnapping(); //don't snap when dragging the whole switch
        }
    }
    
    //move through the keyframes, if both the start and the end have been selected
    //count it as completely selected and let the super class take care of it
    //otherwise if just one of the edges are selected make sure it's unselected
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        if (switchKey->startSelected && switchKey->endSelected) {
            switchKey->startSelected = switchKey->endSelected = false;
            selectKeyframe(switchKey);
        }
        //make sure that if just one of the edges is clicked that the keyframe is *not* selected
		//also make sure it wasn't *just* selected in the last click by checking that it's not 'the' selected key
        else if( (switchKey->startSelected || switchKey->endSelected) && isKeyframeSelected(switchKey)){
			if(selectedKeyframe == switchKey){
				switchKey->startSelected = switchKey->endSelected = false;
			}
			else{
	            deselectKeyframe(switchKey);
			}
        }
    }
	return false;
}

void ofxTLAudioSwitches::unselectAll(){
    ofxTLKeyframes::unselectAll();
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        switchKey->startSelected = switchKey->endSelected = false;
        switchKey->textField.disable();
    }
}

void ofxTLAudioSwitches::updateEdgeDragOffsets(long clickMillis){
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
    	if(switchKey->startSelected){
            switchKey->edgeDragOffset = clickMillis - switchKey->timeRange.min;
        }
        if( switchKey->endSelected){
            switchKey->edgeDragOffset = clickMillis - switchKey->timeRange.max;
        }
    }
}

void ofxTLAudioSwitches::mouseDragged(ofMouseEventArgs& args, long millis){
    if(enteringText)
        return;
    
    //do the normal dragging behavior for selected keyframes
    ofxTLKeyframes::mouseDragged(args, millis);
	
	if(keysAreDraggable){
		//look for any keys with just beginning and ends selected
		//becaues of the logical in the mousePressed, there will never
		//be a selected keyframe with an end selected
		for(int i = 0; i < keyframes.size(); i++){
			ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
			if(switchKey->startSelected){
				switchKey->timeRange.min = millis - switchKey->edgeDragOffset;
				switchKey->time = switchKey->timeRange.min;
			}
			else if(switchKey->endSelected){
				switchKey->timeRange.max = millis - switchKey->edgeDragOffset;
			}
		}
		
		updateTimeRanges();
	}
}

void ofxTLAudioSwitches::mouseMoved(ofMouseEventArgs& args, long millis){
    endHover = startHover = false;
    if(hover && placingSwitch != NULL){
		placingSwitch->timeRange.max = millis;
		return;
	}
	
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        if(abs(switchKey->display.x - args.x) < 10.0 && bounds.inside(args.x,args.y)){
            hoverKeyframe = switchKey;
            startHover = true;
            return; //return cancels call to parent
        }
        float endEdge = switchKey->display.x+switchKey->display.width;
        if(abs(endEdge - args.x) < 10.0 && bounds.inside(args.x,args.y)){
            hoverKeyframe = switchKey;
            endHover = true;
            return; //cancels call to parent
        }
    }
    ofxTLKeyframes::mouseMoved(args, millis);
}

void ofxTLAudioSwitches::nudgeBy(ofVec2f nudgePercent){
	//super class nudge for selected keys
	ofxTLKeyframes::nudgeBy(nudgePercent);
	
	for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
        if(switchKey->startSelected){
            switchKey->timeRange.min += nudgePercent.x*timeline->getDurationInMilliseconds();
            switchKey->time = switchKey->timeRange.min;
        }
        else if(switchKey->endSelected){
            switchKey->timeRange.max += nudgePercent.x*timeline->getDurationInMilliseconds();
		}
	}
	
	updateTimeRanges();
}

//needed to sync the time ranges from pasted keys
void ofxTLAudioSwitches::pasteSent(string pasteboard){
	ofxTLKeyframes::pasteSent(pasteboard);
	updateTimeRanges();
}

//This is called after dragging or nudging, and let's us make sure
void ofxTLAudioSwitches::updateTimeRanges(){
	
    //the superclass will move the ->time value with the drag
    //so we look at the selected keyframes values and see if their changed
    //if so update both the min and the max time so the control moves as a block
	for(int i = 0; i < selectedKeyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)selectedKeyframes[i];
        long dif = switchKey->time - switchKey->timeRange.min;
        switchKey->timeRange.min = switchKey->time;
        switchKey->timeRange.max += dif;
    }
    
	//we also want ot make sure that any nudges or drags that happened to only edge values
	//reversed the min/max relationship value and swap them really quick
    for(int i = 0; i < keyframes.size(); i++){
        //check to see if the user reversed the value and swap them really quick
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
		if(switchKey->timeRange.min > switchKey->timeRange.max){
            float tempPos = switchKey->timeRange.max;
            switchKey->timeRange.max = switchKey->timeRange.min;
            switchKey->timeRange.min = switchKey->time = tempPos;
            bool tempSelect = switchKey->startSelected;
            switchKey->startSelected = switchKey->endSelected;
            switchKey->endSelected = tempSelect;
        }
        switchKey -> shouldRecomputePreview = true;
    }

    if( isOn() && getIsPlaying() ){
        playOnUpdate = true;
    }
    else if( !isOn() && getIsPlaying() ){
        stopOnUpdate = true;
    }
	
    //TODO: no overlaps!!
}

void ofxTLAudioSwitches::mouseReleased(ofMouseEventArgs& args, long millis){
    //if we didn't click on a text field and we are entering txt
    //take off the typing mode. Hitting enter will also do this
    if(enteringText){
		//if we clicked outside of the rect, definitely deslect everything
		if(clickedTextField == NULL && !ofGetModifierSelection()){
			for(int i = 0; i < selectedKeyframes.size(); i++){
				((ofxTLAudioSwitch*)selectedKeyframes[i])->textField.endEditing();
			}
			enteringText = false;
		}
		//otherwise check if still have a selection
		else{
			enteringText = false;
			for(int i = 0; i < selectedKeyframes.size(); i++){
				enteringText = enteringText || ((ofxTLAudioSwitch*)selectedKeyframes[i])->textField.getIsEditing();
			}
		}
        
		if(!enteringText){
			timeline->dismissedModalContent();
			timeline->flagTrackModified(this);
		}
	} else {
        ofxTLKeyframes::mouseReleased(args, millis);
    }
}

void ofxTLAudioSwitches::keyPressed(ofKeyEventArgs& args){
	
	if(enteringText){
        //enter key submits the values
        //This could be done be responding to the event from the text field itself...
        if(args.key == OF_KEY_RETURN){
            enteringText = false;
            timeline->dismissedModalContent();
            timeline->flagTrackModified(this);
        }
    } else {
        ofxTLKeyframes::keyPressed(args);
    }
}

void ofxTLAudioSwitches::regionSelected(ofLongRange timeRange, ofRange valueRange){
    for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
    	if(switchKey->timeRange.intersects(timeRange)){
            selectKeyframe(switchKey);
        }
    }
}

void ofxTLAudioSwitches::getSnappingPoints(set<unsigned long long>& points){
	for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
		if (isKeyframeIsInBounds(switchKey) && !isKeyframeSelected(switchKey) &&
            !switchKey->startSelected && !switchKey->endSelected) {
			points.insert(switchKey->timeRange.min);
            points.insert(switchKey->timeRange.max);
		}
	}
}

int ofxTLAudioSwitches::getSelectedItemCount(){
	int numEdgesSelected = 0;
	for(int i = 0; i < keyframes.size(); i++){
        ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
		if(switchKey->startSelected ||switchKey->endSelected){
			numEdgesSelected++;
		}
	}
	return ofxTLKeyframes::getSelectedItemCount() + numEdgesSelected;
}

ofxTLKeyframe* ofxTLAudioSwitches::newKeyframe(){
    ofxTLAudioSwitch* switchKey = new ofxTLAudioSwitch();
    switchKey->textField.setFont(timeline->getFont());

    //in the case of a click, start at the mouse positiion
    //if this is being restored from XML, the next call to restore will override this with what is in the XML
    switchKey->timeRange.min = switchKey->timeRange.max = screenXToMillis(ofGetMouseX());
    switchKey->startSelected = false;
    switchKey->endSelected   = true; //true so you can drag the range to start with
	
	//for just placing a switch we'll be able to decide the end position
	placingSwitch = switchKey;

   switchKey->shouldRecomputePreview = true;
	
    return switchKey;
}

void ofxTLAudioSwitches::restoreKeyframe(ofxTLKeyframe* key, ofxXmlSettings& xmlStore){
    //pull the saved time into min, and our custom max value
    ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)key;
    switchKey->textField.text = xmlStore.getValue("switchName", "");

    switchKey->timeRange.min = switchKey->time;
    //
    string timecode = xmlStore.getValue("max", "00:00:00:000");
    if(timecode.find(":") == string::npos){
        switchKey->timeRange.max = ofToFloat(timecode) * timeline->getDurationInMilliseconds(); //Legacy max of 0-1
    }
    else{
		switchKey->timeRange.max = timeline->getTimecode().millisForTimecode(timecode);
    }
    //this is so freshly restored keys won't have ends selected but click keys will
    switchKey->startSelected = switchKey->endSelected = false;
	
	//a bit of a hack, but if 
	placingSwitch = NULL;
}

void ofxTLAudioSwitches::storeKeyframe(ofxTLKeyframe* key, ofxXmlSettings& xmlStore){
    //push the time range into X/Y
    ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch* )key;
    xmlStore.addValue("switchName", switchKey->textField.text);
    switchKey->time = switchKey->timeRange.min;
	xmlStore.addValue("max", timeline->getTimecode().timecodeForMillis(switchKey->timeRange.max));
}

void ofxTLAudioSwitches::willDeleteKeyframe(ofxTLKeyframe* keyframe){
	ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch* )keyframe;
	if(switchKey->textField.getIsEditing()){
		timeline->dismissedModalContent();
		timeline->flagTrackModified(this);
	}
	switchKey->textField.disable();
}

ofxTLKeyframe* ofxTLAudioSwitches::keyframeAtScreenpoint(ofVec2f p){
	for(int i = 0; i < keyframes.size(); i++){
		ofxTLAudioSwitch* switchKey = (ofxTLAudioSwitch*)keyframes[i];
    	if(switchKey->display.inside(p)){
            return switchKey;
        }
    }
    return NULL;
}

string ofxTLAudioSwitches::getTrackType(){
    return "Switches";
}
