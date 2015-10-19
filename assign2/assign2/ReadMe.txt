Amy DiGiovanni
Assignment 2

The program usage is:
assign2 <trackfile> <skybox top texture> <skybox bottom texture> <skybox left texture> 
<skybox right texture> <skybox front texture> <skybox back texture>

Spline Rendering:
	For this part of the assignment, I used the recursive
	subdivision algorithm. The step size is inversely proportional
	to the number of control points. This is to ensure that performance
	does not suffer for larger splines, and that the coaster doesn't
	go too fast for smaller splines.
	If multiple splines are used as input, the program will render
	each one in the same space.
Ground and Sky:
	To create the environment, I used a space skybox. The "ground" is the bottom of
	the skybox.
The Ride:
	To get a continuous camera orientation, I used an arbitrary
	vector to compute a local coordinate system for the first point,
	then computed each subsequent coordinate system based on the previous.
	After reaching the end of a spline, the program jumps to the next
	one if there is one, otherwise it goes back to the beginning of
	the first spline.
Cross-Section:
	The cross-section uses the local coordinate system from the 
	previous part to create two rails on either side of the spline.

Controls:
	v - switch to vertex rendering mode for track
	w - switch to wireframe rendering mode for track
	s - switch to solid rendering mode for track
	u - increase speed
	j - decrease speed (cannot go below 0)

The animation is a sequence of .jpg's named "000.jpg" to "312.jpg".