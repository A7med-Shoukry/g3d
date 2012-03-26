import javax.swing.*;
import java.awt.event.*;
import java.awt.*;

public class Window extends JFrame {

    public static void main(String args[]) {
        new Window();
    }
    
    public Window() {
        setVisible(true);
        setTitle("G3D OS GUI Theme Creator");
        setSize(145, 512);
        
        addWindowListener(new WindowAdapter() {
            public void windowClosing(WindowEvent e) {
                System.exit(0);
            }
        } );
        
        final JPanel pane = (JPanel)getContentPane();
        pane.setLayout(new FlowLayout());
        pane.setBackground(Color.WHITE);

        {
            ButtonGroup g = new ButtonGroup();
            JRadioButton w = new JRadioButton("White");
            JRadioButton b = new JRadioButton("Black");
            pane.add(w);
            pane.add(b);
            g.add(w);
            g.add(b);
            w.setSelected(true);
            w.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    pane.setBackground(Color.WHITE);
                }
            });
            b.addActionListener(new ActionListener() {
                public void actionPerformed(ActionEvent e) {
                    pane.setBackground(Color.BLACK);
                }
            });
        }
            
        // check buttons
        {
            pane.add(new JCheckBox());
            
            JCheckBox b = new JCheckBox();
            b.setSelected(true);
            pane.add(b);
            
            b = new JCheckBox();
            b.setEnabled(false);
            pane.add(b);
            
            b = new JCheckBox();
            b.setEnabled(false);
            b.setSelected(true);  
            pane.add(b);
        }

        // Radio buttons
        {
            pane.add(new JRadioButton());
            
            JRadioButton b = new JRadioButton();
            b.setSelected(true);
            pane.add(b);
            
            b = new JRadioButton();
            b.setEnabled(false);
            pane.add(b);
            
            b = new JRadioButton();
            b.setEnabled(false);
            b.setSelected(true);
            pane.add(b);
        }
        
        // Regular buttons
        {
            // The buttons should be 19x19, but sizing them that small causes OS X
            // to change their appearance to square corners
            Dimension d = new Dimension(40, 40);
            JButton b;
            
            b = new JButton("");
            b.setPreferredSize(d);
            pane.add(b);

            b = new JButton("");
//            b.setPreferredSize(d); // Prevents default button
            b.setDefaultCapable(true);
            b.setEnabled(false);
            pane.add(b);            
            pane.getRootPane().setDefaultButton(b);

            b = new JButton("");
            b.setPreferredSize(d);
            b.setEnabled(false);
            pane.add(b);
        }
        
        // Panes
        {
            JComboBox b = new JComboBox();
            pane.add(b);

            b = new JComboBox();
            b.setEnabled(false);
            pane.add(b);
        }
    }
    
    

}
