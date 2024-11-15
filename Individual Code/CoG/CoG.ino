float platformFrontWeight = 0;
float platformLeftWeight=  0;
float platformRightWeight = 0;

void setup() {
  // put your setup code here, to run once:

}

void loop() {
  // put your main code here, to run repeatedly:

}

void CaclulateCG(float nosePos, float dim, float frontWeight, float leftWeight, float rightWeight){
    int nosePos[2] = {14,7};
    int frontScalePos[2] = {14,7};
    int leftScalePos[2] = {26,0};
    int rightScalePos[2] = {26,14};
    int centerOfPlatform[2] = {20,7}; //needs to be tested/verified it's symmetrical

    //total weight of the model - total weight of the platform : calibration
    float modelWeight = (frontWeight+leftWeight+rightWeight)-(platformFrontWeight+platformLeftWeight+platformRightWeight);

    //xcg = (50*frontScalePos[0]+leftScalePos[0]*50+rightScalePos[0]*50)/(150)
    float xcg = ((frontWeight-platformFrontWeight)*frontScalePos[0]+(leftWeight-platformLeftWeight)*leftScalePos[0]+(rightWeight-platformRightWeight)*rightScalePos[0])/(modelWeight);
    //ycg = (50*frontScalePos[1]+leftScalePos[1]*50+rightScalePos[1]*50)/(150)
    float ycg = ((frontWeight-platformFrontWeight)*frontScalePos[1]+(leftWeight-platformLeftWeight)*leftScalePos[1]+(rightWeight-platformRightWeight)*rightScalePos[1])/(modelWeight);
    
    
    //adjustment values of where the model needs to be
    float adjustmentX = xcg - centerOfPlatform[0];
    float newNoseX = nosePos[0] - adjustmentX;
    float adjustmentY = ycg - centerOfPlatform[1];
    float newNoseY = nosePos[1] - adjustmentY;
}
