/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package machine;

import gui.Screen;
import java.awt.image.BufferedImage;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JLabel;
import z80emu.Z80CPU;
import z80emu.Z80CTC;
import z80emu.Z80ExternalException;
import z80emu.Z80IOSystem;
import z80emu.Z80Memory;

/**
 *
 * @author Administrator
 */
public class Vzlet extends Thread 
       implements Z80Memory, Z80IOSystem {
    
    private final int VERSION     = 0x01; //version 0.1
    
    private Screen scr;
    private BufferedImage img;
    private Config cfg;
    private Keyboard key;
    private Memory mem;
    private Z80CPU cpu;
    private Z80CTC ctc;
    
    public JLabel Led1, Led2, Led3, Led4;
    
    private boolean paused;
    private boolean monitor;
    private boolean second;
    private int m1pg, rdpg, wrpg;
    
    public Vzlet() {
        cfg = new Config(); cfg.LoadConfig();
        img = new BufferedImage(720, 288, BufferedImage.TYPE_INT_RGB);
        mem = new Memory(cfg);
        key = new Keyboard();
        cpu = new Z80CPU(this, this);
        ctc = new Z80CTC("CTC1");
        
        cpu.setMaxSpeedKHz(2000);
        
        paused = true;

        Reset(true);
    }
    
    public Config getConfig() {
        return cfg;
    }
    
    public Keyboard getKeyboard() {
        return key;
    }
    
    public void setScreen(Screen screen) {
        scr = screen;
    }
    
    public BufferedImage getImage() {
        return img;
    }
    
    public void setLed1(JLabel led) {
        Led1 = led;
    }
    
    public void setLed2(JLabel led) {
        Led2 = led;
    }
    
    public void setLed3(JLabel led) {
        Led3 = led;
    }
    
    public void setLed4(JLabel led) {
        Led4 = led;
    }
    
    public final void Reset(boolean dirty) {
        monitor = true;
        second = false;
        m1pg = rdpg = wrpg = 0;

        mem.reset(dirty);
        key.reset();
        cpu.resetCPU(dirty);
    }
    
    public final void Nmi() {
//        mem.dumpRam("dump.bin", 0, 7);
    }

    public void startEmulation() {
        if (!paused) return;
        
        scr.repaint();
        cpu.wakeUp();
        paused = false;
       }
    
    public void stopEmulation() {
        if (paused) return;
        
        cpu.waitFor();
        paused = true;
    }
    
    public boolean isPaused() {
        return paused;
    }
    
    public void ms20() {        
        if (!paused) {
            scr.repaint();
        }  
    }
    
    @Override
    public void run() {
        try {
            cpu.run();
        } catch (Z80ExternalException ex) {
            Logger.getLogger(Vzlet.class.getName()).log(Level.SEVERE, null, ex);
        }
        startEmulation();
        
        boolean forever = true;
        while(forever) {
            try {
                sleep(Long.MAX_VALUE);
            } catch (InterruptedException ex) {
                Logger.getLogger(Vzlet.class.getName()).log(Level.SEVERE, null, ex);
            }
        }        
    }    

    @Override
    public int readIOByte(int port) {
        int value = 0xff;
        System.out.println(String.format("In: %04X,%02X (%04X)", port,value,cpu.getRegPC()));
        return value;
    }

    @Override
    public void writeIOByte(int port, int value) {
        switch(port & 0xff) {
            case 0xc0: { second = false; break; }
            case 0xc1: { second = true; break; }
            case 0xc2: { second = false; break; }
            case 0xc3: { second = true; break; }            
            case 0xec: {    // page selection
                monitor = ((value & 0x80)==0);
                rdpg = 0x30000 & (value << 16);
                wrpg = 0x30000 & (value << 18);
                m1pg = 0x30000 & (value << 20);
                break;
            }
        }
        System.out.println(String.format("Out: %04X,%02X (%04X)", port,value,cpu.getRegPC()));
    }

    @Override
    public boolean setMemByte(int addr, int value) {
        System.out.println(String.format("setMemByte: %04X,%02X (%04X)", addr,value,cpu.getRegPC()));
        writeMemByte(addr, value);
        return true;
    }

    @Override
    public int readMemByte(int addr, boolean m1) {
        int value = 0;
        if (monitor && (addr<0x4000)) {
            value = mem.readMonitor(addr);
        }
        else {
            if (second) {
                
            }
            else {
                value = m1 ? mem.readRam(addr|m1pg) : mem.readRam(addr|rdpg);
            }
        }
        System.out.println(String.format("Rd: %05X,%B (%04X)", addr,m1,cpu.getRegPC()));
        return 0xff & value;
    }

    @Override
    public void writeMemByte(int addr, int value) {
        System.out.println(String.format("Wr: %05X,%02X (%04X)", addr,value,cpu.getRegPC()));
        mem.writeMonitor(addr, (byte) value);
    }

    @Override
    public int getMemByte(int addr, boolean m1) {
        System.out.println(String.format("getMemByte: %05X,%B (%04X)", addr,m1,cpu.getRegPC()));
        return readMemByte(addr, m1);
    }

    @Override
    public int getMemWord(int addr) {
        int lsb = getMemByte(addr, false) & 0xff;
        addr = (addr+1) & 0x3ffff;
        return ((getMemByte(addr, false) << 8) & 0xff00 | lsb);
    }

}
