Name : 김혜린
StudentID : 20231371

[How to Compile]
- Operating System: Windows 10 (64-bit)
- Compiler: Microsoft Visual Studio 2022 (MSVC)
- DirectX SDK Version: DirectX 9.0c
- Compilation Method : Select Rebuild Solution in Solution Explorer from an open project, or use the shortcut Ctrl+Shift+B

[How to Execute]
VirtualLego.sln 
Press Space to Start
Right-click the mouse and move to manipulate the white ball
If you touch the bottom wall, the game is over.
If you want to end the game immediately, press esc key.

[Summary of Your Code Modification]
1. CSphere
- hasIntersected() : Use the center coordinates of two balls to return whether two balls are in contact
- hitBy() : when hasIntercepted is true, reset redball's speed
- ballupdate() : Adjust the speed of the ball not to become too slow or too large

2. CWall
- hasIntersected() : Return redball to wall contact
- hitBy() : Reset the speed depending on which wall the redball hit (set it to restart when it hits the bottom)

3. destroyAllLegoBlock() 
- Delete all billiard balls

4. Setup()
- reset billiard board position, ball position

5. Display()
- If restart flag is true, then call setup function again. 
- If complete flag is true, then set the program to end
- Verify that redball is in contact with g_legowall or g_sphere, g_whiteball