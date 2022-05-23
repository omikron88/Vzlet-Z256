/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package machine;

import java.awt.image.BufferedImage;
import java.awt.Graphics;

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
    private int pocit;
    
    Crtc(Vzlet machine, Memory mem) {
        ma = machine;
        m = mem;
        i = ma.getImage();
        regs = new byte[16];
        pocit=0;
    }
    
    public void reset() {
      pocit=0;
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
        int c,d,ad,bd,adr,adl,adp,hh,llen,chrl,blsp,kpos;
        boolean blik,zobk;
        int ink;
        String tx;
        Graphics g = i.getGraphics();

        adr=(this.regs[13]+256*this.regs[12])|0x8000;
        llen=this.regs[1];  // 50h=80                // počet znaků na řádek
        chrl=this.regs[9];  // 0Bh=11                // počet vertikálních linek (počítáno od 0 ?)
        blsp = (this.regs[10]&0x40)==0 ? 16 : 32;    // rychlost blikání
        blik = (this.regs[10]&0x20)==0x20;           // blikat kurzorem
        zobk= !((!blik)&(blsp==32));                 // zobrazit kurzor
        kpos= this.regs[15] + 256*this.regs[14];

        tx="adr: "+adr+"  "+llen+","+chrl+" blik: "+blik+" blsp:"+blsp+" zobk: "+zobk+" kpos:"+kpos;  // pomocná informace
        
        adl=adr;    // odkud začít zobrazovat
        for (int li=0; li<300; li++)
         {
          d=li+ofsy;    //Y souřadnice (0-299)+offset
          adp=adl;      //uschovej začátek mikrořádku
          for (int x=0; x<80; x++)
            {
                ad = m.readVram(adl);
                bd = m.readVram(adl|0x10000);
                adl++;if ((adl& 0xff)==0){ adl=adl+0xf00;}  // posun na další znak
                c = x * 8 + ofsx;   // X souřadnice (0-639)+offset
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
            }
          adl=adp;      //obnov začátek mikrořádku
          adl=adl+0x100;  // posun na další mikrořádek
          if (((adl&0xf00)>>8) > chrl){   // pokud je hotovo, posun na další řádek
                 hh=(adr&0xf00)>>8;
                 adr=adr+llen;
                 if (((adr&0xf00)>>8)!=hh){adr=adr+0xf00;}
                 if (adr>65535){adr=(adr&0x0fff)|0x8000;}
                 adl=adr;
                                      }
         }
        pocit++; if (pocit==2*blsp){pocit=0;}  // zvyš počítadlo frames pro blikání kurzoru - perioda je 2x blsp
        if ((zobk)&((pocit < blsp)&(blik))){    // zobrazit kurzor, pokud je povoleno, a když bliká tak pouze v první půlce režimu 2x blsp
            
                                           }
     g.drawString(tx, 5,290);           // zobraz pomocnou informaci
    }    // paint
}
