// bad implementation
function sleep(milliSeconds){
	var startTime = new Date().getTime();                    // get the current time
	while (new Date().getTime() < startTime + milliSeconds); // hog cpu
}
