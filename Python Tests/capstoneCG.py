def centerOfGravity(nosePos,dim,frontWeight,leftWeight,rightWeight):#relativeNosePosition,frontWeight,leftWeight,rightWeight, x or z

    #relative scale positions from diagram of platform
    nosePos = [14,7]
    frontScalePos = [14,7]
    leftScalePos = [26,0]
    rightScalePos = [26,14]
    centerOfPlatform = [20,7] #needs to be tested/verified it's symmetrical

    #constant values of the platform weight on the sensors, needs to be subtracted out of 
    platformFrontWeight = 0
    platformLeftWeight=  0
    platformRightWeight = 0

    #total weight of the model - total weight of the platform : calibration
    modelWeight = (frontWeight+leftWeight+rightWeight)-(platformFrontWeight+platformLeftWeight+platformRightWeight)

    #xcg = (50*frontScalePos[0]+leftScalePos[0]*50+rightScalePos[0]*50)/(150)
    xcg = ((frontWeight-platformFrontWeight)*frontScalePos[0]+(leftWeight-platformLeftWeight)*leftScalePos[0]+(rightWeight-platformRightWeight)*rightScalePos[0])/(modelWeight)
    #ycg = (50*frontScalePos[1]+leftScalePos[1]*50+rightScalePos[1]*50)/(150)
    ycg = ((frontWeight-platformFrontWeight)*frontScalePos[1]+(leftWeight-platformLeftWeight)*leftScalePos[1]+(rightWeight-platformRightWeight)*rightScalePos[1])/(modelWeight)
    
    
    #adjustment values of where the model needs to be
    adjustmentX = xcg - centerOfPlatform[0]
    newNoseX = nosePos[0] - adjustmentX
    adjustmentY = ycg - centerOfPlatform[1]
    newNoseY = nosePos[1] - adjustmentY

    print(newNoseX)#output to LED screen, balanced nose postion to slide the model to
    print(newNoseY)#should be 7 if balanced properly
    return 0

centerOfGravity(14,"x",50,50,50)
