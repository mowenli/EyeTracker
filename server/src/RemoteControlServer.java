import java.awt.AWTException;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.MouseInfo;
import java.awt.PointerInfo;
import java.awt.Robot;
import java.awt.Toolkit;
import java.awt.datatransfer.Clipboard;
import java.awt.datatransfer.DataFlavor;
import java.awt.datatransfer.StringSelection;
import java.awt.datatransfer.Transferable;
import java.awt.event.InputEvent;
import java.awt.event.KeyEvent;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.net.DatagramPacket;
import java.net.DatagramSocket;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Scanner;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JTextField;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;

public class RemoteControlServer extends JFrame {
	
	private static int port;
	private static double mx;	//�������ĺ�����
	private static double my;	//��������������
	ServerThread serverthread; //��ʼ���߳�
	final JTextField messagebox;
	final JTextField field;
	final JButton stopbutton;
	final JButton startbutton;
	static int menux  =0; //menux�ź��� 0��ʾδ���� 1��ʾ���� 2��ʾ��ͣ
	String message =null;
	String[] messages =null;
	String type =null;
	String info =null;
	public RemoteControlServer(){
		super();
        setTitle("Զ�̿���");
        setSize(230, 300);
        setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        Toolkit toolkit = getToolkit(); // ���Toolkit����
        Dimension dimension = toolkit.getScreenSize(); // ���Dimension����
        int screenHeight = dimension.height; // �����Ļ�ĸ߶�
        int screenWidth = dimension.width; // �����Ļ�Ŀ��
        int frm_Height = this.getHeight(); // ��ô���ĸ߶�
        int frm_width = this.getWidth(); // ��ô���Ŀ��
        setLocation((screenWidth - frm_width) / 2, (screenHeight - frm_Height) / 2); // ʹ�ô��������ʾ
        getContentPane().setLayout(null);
        final JLabel label = new JLabel();
        try {
			label.setText("����IP��"+InetAddress.getLocalHost().getHostAddress());
		} catch (UnknownHostException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
        label.setBounds(10, 20, 300, 25);
        Font font = new Font("SimSun", Font.PLAIN, 16);
        label.setFont(font);
        getContentPane().add(label);
        
        final JLabel label2 =new JLabel();
    	label2.setText("������˿ںţ�");
    	label2.setBounds(10, 50, 100, 25);
    	getContentPane().add(label2);
    	
    	field = new JTextField();
    	field.setBounds(110,50,90,25);
    	getContentPane().add(field);
        
    	startbutton = new JButton();
    	startbutton.setText("����");
    	startbutton.setBounds(10,90, 80, 25);
    	getContentPane().add(startbutton);
    	
    	stopbutton = new JButton();
    	stopbutton.setText("ֹͣ");
    	stopbutton.setEnabled(false);
    	stopbutton.setBounds(120,90, 80, 25);
    	getContentPane().add(stopbutton);
    	
        final JLabel label3 =new JLabel();
    	label3.setText("�����ֻ������� ����IP �� �˿ں�");
    	label3.setBounds(10, 120, 280, 25);
    	getContentPane().add(label3);
    	
    	final JLabel label4 =new JLabel();
    	label4.setText("���յ���Ϣ��");
    	label4.setBounds(10, 150, 280, 20);
    	getContentPane().add(label4);
    	
    	messagebox = new JTextField();
    	messagebox.setBounds(10,180,190,25);
    	messagebox.enable(false);
    	getContentPane().add(messagebox);
    	
    	startbutton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
            	String str  = field.getText().trim();
            	int num;
            	if(str.equals("")){
            		JOptionPane.showMessageDialog(null,"������Ϣ����Ϊ��");
            		return;
            	}
            	try{
            		num = Integer.parseInt(str);
            	}catch(Exception e){
            		JOptionPane.showMessageDialog(null,"�˿ں�Ӧ��Ϊ����");
            		return;
            	}
            	if(num<0||num>65535){
            		JOptionPane.showMessageDialog(null,"�˿ں�Ӧ�ô���0С��65535");
            		return;
            	}
            	port=num;
            	stopbutton.setEnabled(true);
            	startbutton.setEnabled(false);
            	start();
            	
            }
        });
    	
    	stopbutton.addActionListener(new java.awt.event.ActionListener() {
            public void actionPerformed(java.awt.event.ActionEvent evt) {
 //           	startbutton.setEnabled(false);
            	stop();
            	stopbutton.setEnabled(false);
            	startbutton.setEnabled(true);
            }
    	 });
		
    	setVisible(true);
	}
	
	public static void main(String[] args) {
		try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
		} catch (ClassNotFoundException | InstantiationException 
				| IllegalAccessException | UnsupportedLookAndFeelException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		java.awt.EventQueue.invokeLater(new Runnable() {
            public void run() {
            	new RemoteControlServer();
            }
        });
	
	}
		
	public void start(){
		if (menux == 0) {   //menux�ź��� 0��ʾδ���� 1��ʾ���� 2��ʾ��ͣ
			serverthread = new ServerThread();
			serverthread.start();
			menux = 1;
			messagebox.setText("������Ϣ����");
			field.setEditable(false);
		} else if (menux == 2) {
			serverthread.resume();
			menux = 1;
			messagebox.setText("�ָ���Ϣ����");
		}
	}
	
	public void stop() {
		if(menux==1){
			serverthread.suspend();
			menux=2;
			messagebox.setText("��ͣ��Ϣ����");
		}
		
	}
	
	
	public class ServerThread extends Thread {
    	public void run(){
    		try {
    			//����һ��DatagramSocket���󣬲�ָ�������Ķ˿ں�
    			DatagramSocket socket;
    			try {
    				socket = new DatagramSocket(port);
    			} catch(Exception e) {
    				messagebox.setText("�˿ڱ�ʹ��,������˿�");
    				startbutton.setEnabled(true);
    				stopbutton.setEnabled(false);
    				menux = 0;
    				field.setEditable(true);
    				return;
    			}
				byte data [] = new byte[1024];
				//����һ���յ�DatagramPacket����
				DatagramPacket packet = new DatagramPacket(data, data.length);
				//ʹ��receive�������տͻ��������͵�����
				System.out.println("�����˿ڼ���"+socket.getLocalPort());
				while(true) {
					socket.receive(packet);
					message = new String(packet.getData(),packet.getOffset(),packet.getLength());
					System.out.println("message--->" + message);
					
					InetAddress address = null;
			        int port = 8800;
			        byte[] data2 = null;
			        DatagramPacket packet2 = null;
					address = packet.getAddress();
		            port = packet.getPort();
		            data2 = ("receive: " + info).getBytes();
		            packet2 = new DatagramPacket(data2, data2.length, address, port);
		            socket.send(packet2);
					
					messagebox.setText(message);
					messages = message.split(":");
					if (messages.length >= 2) {
						type = messages[0];
						info = messages[1];
						if(type.equals("mouse"))
							MouseMove(info);
						if(type.equals("leftButton"))
							LeftButton(info);
						if(type.equals("rightButton"))
							RightButton(info);
						if(type.equals("mousewheel"))
							MouseWheel(info);
						if(type.equals("keyboard"))
							KeyBoard(info);
					}
				}
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
    	}
    	
    	public void MouseMove(String info){
    		String args[]=info.split(",");
    		String x= args[0];
    		String y= args[1];
			float px = Float.valueOf(x);
			float py = Float.valueOf(y);
			
			PointerInfo pinfo = MouseInfo.getPointerInfo();		//�õ���������
			java.awt.Point p = pinfo.getLocation();
			mx=p.getX();	//�õ���ǰ������������
			my=p.getY();
			java.awt.Robot robot;
			try {
				robot = new Robot();
//				System.out.println(mx+","+my);
//				System.out.println(px+","+py);
				robot.mouseMove((int)mx+(int)px,(int)my+(int)py);
			} catch (AWTException e) {
				e.printStackTrace();
			}
			
    	}
    	
    	public void LeftButton(String info) throws AWTException{
    		java.awt.Robot robot = new Robot();
    		if(info.equals("down"))
				robot.mousePress(InputEvent.BUTTON1_MASK);				
    		else if(info.equals("release"))
    			robot.mouseRelease(InputEvent.BUTTON1_MASK);
    		else if(info.equals("up"))
    			robot.mouseRelease(InputEvent.BUTTON1_MASK);
    		else if(info.equals("click")){
    			robot.mousePress(InputEvent.BUTTON1_MASK);
    			robot.mouseRelease(InputEvent.BUTTON1_MASK);
    		}
    	}
    	
    	public void RightButton(String info) throws AWTException{
    		java.awt.Robot robot = new Robot();
    		if(info.equals("down"))
				robot.mousePress(InputEvent.BUTTON3_MASK);				
    		else if(info.equals("release"))
    			robot.mouseRelease(InputEvent.BUTTON3_MASK);
    		else if(info.equals("up"))
    			robot.mouseRelease(InputEvent.BUTTON3_MASK);
    	}
    	
    	public void MouseWheel(String info)throws AWTException{
    		java.awt.Robot robot = new Robot();
    		float num = Float.valueOf(info);
    		if(num>0)
    			robot.mouseWheel(1);
    		else
    			robot.mouseWheel(-1);
    	}
    	
    	public void KeyBoard(String info)throws AWTException{
    		String args[]=info.split(",");
    		String type=null;
    		String cont=null;
    		String keystate =null;
    		java.awt.Robot robot = new Robot();
    		if(args.length==2){
    			type = args[0];
    			cont = args[1];
    		}
    		if(args.length==3){
    			type = args[0];
    			cont = args[1];
    			keystate = args[2];
    		}
    		
    		if(type.equals("message")){
    			Clipboard cb = Toolkit.getDefaultToolkit().getSystemClipboard();
    			cb.setContents(new StringSelection(cont), null);//����ճ����
    
    			
    			robot.keyPress(KeyEvent.VK_CONTROL);
    			robot.keyPress(KeyEvent.VK_V);
    			robot.keyRelease(KeyEvent.VK_CONTROL);
    			robot.keyRelease(KeyEvent.VK_V);
    		}else if(type.equals("key")){
    			if(cont.equals("BackSpace")){
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_BACK_SPACE);
    					robot.keyRelease(KeyEvent.VK_BACK_SPACE);
    				}
    			}
    			if(cont.equals("Enter")){
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_ENTER);
    					robot.keyRelease(KeyEvent.VK_ENTER);
    				}
    			}
    			if(cont.equals("Up")){
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_UP);
    					robot.keyRelease(KeyEvent.VK_UP);
    				}
    					
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_UP);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_UP);
    			}
    			if(cont.equals("Down")){
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_DOWN);
    					robot.keyRelease(KeyEvent.VK_DOWN);
    				}
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_DOWN);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_DOWN);
    			}
    			if(cont.equals("Left")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_LEFT);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_LEFT);
    			}
    			if(cont.equals("Right")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_RIGHT);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_RIGHT);
    			}
    			if(cont.equals("W")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_W);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_W);
    			}
    			if(cont.equals("S")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_S);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_S);
    			}
    			if(cont.equals("A")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_A);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_A);
    			}
    			if(cont.equals("S")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_S);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_S);
    			}
    			
    			if(cont.equals("Ctrl")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_CONTROL);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_CONTROL);
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_CONTROL);
    					robot.keyRelease(KeyEvent.VK_CONTROL);
    				}
    			}
    			
    			if(cont.equals("Z")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_Z);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_Z);
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_Z);
    					robot.keyRelease(KeyEvent.VK_Z);
    				}
    			}
    			
    			if(cont.equals("Space")){
    				if(keystate.equals("down"))
    					robot.keyPress(KeyEvent.VK_SPACE);
    				if(keystate.equals("up"))
    					robot.keyRelease(KeyEvent.VK_SPACE);
    				if(keystate.equals("click")){
    					robot.keyPress(KeyEvent.VK_SPACE);
    					robot.keyRelease(KeyEvent.VK_SPACE);
    				}
    			}
    		}else if(type.equals("dosmessage")){
    			Clipboard cb = Toolkit.getDefaultToolkit().getSystemClipboard();
    			cb.setContents(new StringSelection(cont), null);//����ճ����
    			robot.mousePress(InputEvent.BUTTON3_MASK);
    			robot.mouseRelease(InputEvent.BUTTON3_MASK);
				robot.keyPress(KeyEvent.VK_P);
				robot.keyRelease(KeyEvent.VK_P);
    		}
    	}
	}

}
