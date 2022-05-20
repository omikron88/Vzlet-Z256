/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package machine;

import java.awt.image.BufferedImage;

/**
 *
 * @author macura
 */
public class Crtc {

    private final int ofsx = 40;
    private final int ofsy = 0;
    
    private final int pal[] = {0x000000, 0x505050, 0xA0A0A0, 0xF0F0F0};   // index GRBI

    private Vzlet ma;
    private Memory m;
    private BufferedImage i;
    private int index;
    private byte[] regs;
    
    Crtc(Vzlet machine, Memory mem) {
        ma = machine;
        m = mem;
        i = ma.getImage();
        regs = new byte[16];
    }
    
    public void reset() {

    }

    public void setIndex(int ind) {
        index = ind & 0x0f;
    }
    
    public void setReg(int val) {
        regs[index] = (byte) val;
    }
    
    public int getReg() {
        return 0xff & regs[index];
    }
    
    public void paint() {
        int c,d,ad,bd,adr;
        int ink;
        
        for (int y=0; y<299; y++) {
            adr = 0x8000+y*80;          // zde netuším, na jakých adresách začínají řádky
            d = y + ofsy;
            for (int x=0; x<80; x++) {
                ad = m.readVram(adr) & 0xff;
                bd = m.readVram(adr|0x10000) & 0xff;
                adr++;
                c = x * 8 + ofsx;
                ink = pal[((ad&0x80)>>7)|((bd&0x80)>>6)];
                i.setRGB(c++, d, ink);
                ink = pal[((ad&0x40)>>6)|((bd&0x40)>>5)];
                i.setRGB(c++, d, ink);
                ink = pal[((ad&0x20)>>5)|((bd&0x20)>>4)];
                i.setRGB(c++, d, ink);
                ink = pal[((ad&0x10)>>4)|((bd&0x10)>>3)];
                i.setRGB(c++, d, ink);
                ink = pal[((ad&0x08)>>3)|((bd&0x08)>>2)];
                i.setRGB(c++, d, ink);
                ink = pal[((ad&0x04)>>2)|((bd&0x04)>>1)];
                i.setRGB(c++, d, ink);
                ink = pal[((ad&0x02)>>1)|(bd&0x02)];
                i.setRGB(c++, d, ink);
                ink = pal[(ad&0x01)|((bd&0x01)<<1)];
                i.setRGB(c++, d, ink);
            }  //x
        }     //y

    }    
}
