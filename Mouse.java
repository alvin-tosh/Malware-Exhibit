// Developer: Palpatine
// Randomly moves the mouse pointer, & clicks different places on the screen.

import java.awt.Robot;
import java.awt.event.InputEvent;
import java.util.Random;

public class out_of_control {
    public static void main(String[] args) throws Exception {
    	Robot robot = new Robot();
    	Random random = new Random();
    	while(true){
    		robot.mouseMove(random.nextInt(1200), random.nextInt(700));
        
    		//click(robot); // don't un-comment, may cause harm.
    		Thread.sleep(300);
    	}
    }
    
    private static void click(Robot robot){
        robot.mousePress(InputEvent.BUTTON1_MASK);robot.mouseRelease(InputEvent.BUTTON1_MASK);
        robot.mousePress(InputEvent.BUTTON1_MASK);robot.mouseRelease(InputEvent.BUTTON1_MASK);
    }
}
