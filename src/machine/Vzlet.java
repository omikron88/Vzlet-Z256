/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package machine;

import gui.Screen;
import java.awt.image.BufferedImage;
import java.util.ArrayList;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JLabel;
import z80emu.Z80CPU;
import z80emu.Z80CTC;
import z80emu.Z80ExternalException;
import z80emu.Z80IOSystem;
import z80emu.Z80InterruptSource;
import z80emu.Z80Memory;
import z80emu.Z80PIO;
import z80emu.Z80TStatesListener;

/**
 *
 * @author Administrator
 */
public class Vzlet extends Thread 
       implements Z80Memory, Z80IOSystem, Z80TStatesListener {
    
    private final int VERSION     = 0x01; //version 0.1
    
    private Screen scr;
    private BufferedImage img;
    private Config cfg;
    private Keyboard key;
    private Memory mem;
    private Crtc crtc;
    private Z80CPU cpu;
    private Z80CTC ctc, ctcfd;
    private Z80PIO piofd;
    
    public JLabel Led1, Led2, Led3, Led4;
    
    private boolean paused;
    private boolean monitor;
    private boolean second;
    private int m1pg, rdpg, wrpg;
    private int tsVideo, tsCtc, tsCtcfd; // T couunters
    
    public Vzlet() {
        cfg = new Config(); cfg.LoadConfig();
        img = new BufferedImage(720, 300, BufferedImage.TYPE_INT_RGB);
        mem = new Memory(cfg);
        key = new Keyboard();
        crtc = new Crtc(this, mem);
        cpu = new Z80CPU(this, this);
        ctc = new Z80CTC("CTC cpu");
        ctcfd = new Z80CTC("CTC fdc");
        piofd = new Z80PIO("PIO fdc");
        
        java.util.List<Z80InterruptSource> iSources
				= new ArrayList<Z80InterruptSource>();
        iSources.add(ctc);
        iSources.add(ctcfd);
        iSources.add(piofd);
        
        cpu.setMaxSpeedKHz(2000);
        cpu.setInterruptSources(iSources.toArray( new Z80InterruptSource[ iSources.size() ] ));
        cpu.addTStatesListener(this);
        cpu.addTStatesListener(ctc);
        cpu.addTStatesListener(ctcfd);
        
        key.setPIO(piofd);
        
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
        crtc.reset();
        cpu.resetCPU(dirty);
        ctc.reset(dirty);
        ctcfd.reset(dirty);
        piofd.reset(dirty);
    }
    
    public final void Nmi() {
//        mem.dumpRam("dump.bin", 0, 63);
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

    private int rdVideo(int pg, int addr) {
        int val = 0;
        switch(pg >>> 16) {
            case 0: { val = mem.readVideo(addr & 0x1fff); break; } // Video Eprom is 8KB 
            case 1: { val = mem.readVram(addr); break; }          // First 64KB of Vram 
            case 2: { val = mem.readVram(addr | 0x10000); break; }// Second 64KB of Vram 
            case 3: { // Registers of CRTC
                if ((addr & 1)==0) {
                    val = 0xff; // reading index is nonsence
                }
                else {
                    val = crtc.getReg(); // read register 
                }
                break;
            } 
        } //switch
        return val;
    }    
    
    private void wrVideo(int pg, int addr, int val) {
        switch(pg >>> 16) {
            case 0: { break; } // Video Eprom cannot be written 
            case 1: { mem.writeVram(addr, (byte) val); break; } // First 64KB of Vram 
            case 2: { mem.writeVram(addr | 0x10000, (byte) val); break; } // Second 64KB of Vram 
            case 3: { // Registers of CRTC
                if ((addr & 1)==0) {
                    crtc.setIndex(val); // index
                }
                else {
                    crtc.setReg(val); // register
                }
                break;
            } 
        } //switch        
    }

    @Override
    public int readIOByte(int port) {
        int value = 0xff;
        switch(port & 0xff) {
            case 0xD4: { value = piofd.readPortA();break; }
            case 0xD5: { value = piofd.readPortB(); System.out.println(String.format("In: %04X,%02X (%04X)", port,value,cpu.getRegPC())); break; }
            case 0xD6: { value = piofd.readControlA(); break; }
            case 0xD7: { value = piofd.readControlB(); break; }
            case 0xD8: { value = ctcfd.read(0); break; }
            case 0xD9: { value = ctcfd.read(1); break; }
            case 0xDA: { value = ctcfd.read(2); break; }
            case 0xDB: { value = ctcfd.read(3); break; }
            case 0xF4: { value = ctc.read(0); break; }
            case 0xF5: { value = ctc.read(1); break; }
            case 0xF6: { value = ctc.read(2); break; }
            case 0xF7: { value = ctc.read(3); break; }
        }         
//        System.out.println(String.format("In: %04X,%02X (%04X)", port,value,cpu.getRegPC()));
        return value;
    }

    @Override
    public void writeIOByte(int port, int value) {
        switch(port & 0xff) {
            case 0xc0: { second = false; break; }
            case 0xc1: { second = true; break; }
            case 0xc2: { second = false; break; }
            case 0xc3: { second = true; break; }
            case 0xd0: {  System.out.println(String.format("Out: %04X,%02X (%04X)", port,value,cpu.getRegPC())); break; }
            case 0xD4: { piofd.writePortA(value); break; }
            case 0xD5: { piofd.writePortB(value); break; }
            case 0xD6: { piofd.writeControlA(value); break; }
            case 0xD7: { piofd.writeControlB(value); break; }
            case 0xD8: { ctcfd.write(0, value); break; }
            case 0xD9: { ctcfd.write(1, value); break; }
            case 0xDA: { ctcfd.write(2, value); break; }
            case 0xDB: { ctcfd.write(3, value); break; }
            case 0xF4: { ctc.write(0, value); break; }
            case 0xF5: { ctc.write(1, value); break; }
            case 0xF6: { ctc.write(2, value); break; }
            case 0xF7: { ctc.write(3, value); break; }
            case 0xfc: {    // page selection
                monitor = ((value & 0x80)==0);
                rdpg = 0x30000 & (value << 16);
                wrpg = 0x30000 & (value << 14);
                m1pg = 0x30000 & (value << 12);
                break;
            }
        } //switch
//        System.out.println(String.format("Out: %04X,%02X (%04X)", port,value,cpu.getRegPC()));
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
        if (monitor && (addr<0x4000)) { // CPU ROM and SRAM
            value = mem.readMonitor(addr);
        }
        else {
            if (second) {  // VIDEO má taky 4 stránky !!!
                value = m1 ? rdVideo(m1pg, addr) : rdVideo(rdpg, addr);
            }
            else { // hlavní přídavná paměť 256kB (4 stránky po 64kB)
                value = m1 ? mem.readRam(addr|m1pg) : mem.readRam(addr|rdpg);
            }
        }
//        System.out.println(String.format("Rd: %04X,%02X,%B (%04X)", addr,0xff & value,m1,cpu.getRegPC()));
        return 0xff & value;
    }

    @Override
    public void writeMemByte(int addr, int value) {
        if (monitor && (addr<0x4000)) { // CPU ROM and SRAM
            mem.writeMonitor(addr, (byte) value);
        }
        else {
            if (second) {  // VIDEO má taky 4 stránky !!!
                wrVideo(wrpg, addr, (byte) value);
            }
            else { // hlavní přídavná paměť 256kB (4 stránky po 64kB)
                mem.writeRam(addr|wrpg, (byte) value);
            }
        }
//        System.out.println(String.format("Wr: %04X,%02X (%04X)", addr,value,cpu.getRegPC()));
    }

@Override
    public int getMemByte(int addr, boolean m1) {
        System.out.println(String.format("getMemByte: %04X,%B (%04X)", addr,m1,cpu.getRegPC()));
        return readMemByte(addr, m1);
    }

    @Override
    public int getMemWord(int addr) {
        int lsb = getMemByte(addr, false) & 0xff;
        addr = (addr+1) & 0xffff;
        return ((getMemByte(addr, false) << 8) & 0xff00 | lsb);
    }

    @Override
    public void z80TStatesProcessed(Z80CPU cpu, int tStates) {
            tsVideo += tStates; 
            tsCtc += tStates;
            tsCtcfd += tStates;
            
        if (tsCtc > 26) { // 13T @4MHz is 26T @2MHz 
            tsCtc = 0;
            ctc.externalUpdate(0, 1);
            ctc.externalUpdate(1, 1);
        }

        if (tsCtcfd > 2) { // 1MHz 
            tsCtcfd = 0;
            ctcfd.externalUpdate(0, 1);
        }

        if (tsVideo > 20000) {
            tsVideo = 0;
            crtc.paint();
            scr.repaint();
        }
    }

}
