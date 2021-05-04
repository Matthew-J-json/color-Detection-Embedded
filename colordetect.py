import cv2          #computervision!
import numpy as np  #numpy to do math stuff

img = cv2.imread('source.png',cv2.IMREAD_UNCHANGED)   #read the image in its entirety (e.g. not grayscale)

data = np.reshape(img, (-1,3))                      #reshape the image data array using numpy
data = np.float32(data)                             #convert the image data to float values

criteria = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 10, 1.0) #setting termination criteria for the algorithm used by CV2
flags = cv2.KMEANS_RANDOM_CENTERS                                        #k-means clustering algorithm with random center points
compactness,labels,centers = cv2.kmeans(data,1,None,criteria,10,flags)


#outputting the color data to colordata.csv
with open("colordata.csv", 'w') as f:           
    bgr = centers[0].astype(np.int32)
    
    for i in range(3):
        f.write(str(bgr[2-i]))
        f.write("\n")
