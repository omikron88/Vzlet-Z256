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
    
    private final int pal[] = {0x000000, 0xA0A0A0, 0x505050, 0xF0F0F0};   // index GRBI

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
    
    private int CalAdr(int adre, int row) {
        int result,r1,r2,r3;
        r1=adre & 0xff;  r2=(adre & 0x700)*16; r3=(row & 0xf)*256;
        result=(r1+r2+r3)|0x8000;
        return result;
    }
    
    public void paint() {
        int c,d,ad,bd,adr,adp,hh,llen,chrl,blsp,kpos,kza,kko,por,pmi,kx,ky,kpy;
        boolean blik,zobk;
        int ink;

        adr=((this.regs[13]&0xff)+256*(this.regs[12]&0x3f));   // pouze 14 bitů !!!!
        llen=(this.regs[1]&0xff);  // 50h=80         // počet znaků na řádek
        chrl=(this.regs[9]&0xff);  // 0Bh=11         // počet vertikálních linek (počítáno od 0 ?)
        blsp = (this.regs[10]&0x40)==0 ? 16 : 32;    // rychlost blikání
        blik = (this.regs[10]&0x20)==0x20;           // blikat kurzorem
        zobk= !((!blik)&(blsp==32));                 // zobrazit kurzor
        por= (this.regs[6]&0x7f);                    // počet zobrazovaných řádků
        kpos= (this.regs[15]&0xff) + 256*(this.regs[14]&0x3f);  // pozice kurzoru - pouze 14 bitů !!!!
        kza = (this.regs[10]&0x0f);                  // počáteční linka kurzoru
        kko = (this.regs[11]&0x0f);                  // koncová linka kurzoru
        
        pmi=kx=ky=kpy=0;           // počítadlo mikrořádků - počáteční hodnota (musí být 0 !!)
        for (int li=0; li<300; li++)
         {
          d=li+ofsy;    //Y souřadnice (0-299)+offset
          for (int x=0; x<80; x++)
            {
                if ((adr+x)==kpos){kx=x;ky=kpy;}
                adp=CalAdr(adr+x,pmi);
                ad = m.readVram(adp);
                bd = m.readVram(adp|0x10000);
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
          pmi++;         // posun na další mikrořádek
          if (pmi > chrl){   // pokud je hotovo, posun na další řádek
                 pmi=0;
                 adr=adr+llen;
                 kpy++;
                         }
         }   // linky
        pocit++; if (pocit==2*blsp){pocit=0;}  // zvyš počítadlo frames pro blikání kurzoru - perioda je 2x blsp
        if ((zobk)&((pocit < blsp)&(blik))){   // zobrazit kurzor, pokud je povoleno, a když bliká tak pouze v první půlce režimu 2x blsp
            ad=kpos/llen;bd=kpos-ad*llen;      // řádek,sloupec
            d=ofsy+ky*(chrl+1)+kza;
            c=kx*8+ofsx;
            do {
            c=bd*8+ofsx;
            i.setRGB(c++, d, 0xF0F0F0);i.setRGB(c++, d, 0xF0F0F0);i.setRGB(c++, d, 0xF0F0F0);i.setRGB(c++, d, 0xF0F0F0);   // 4x
            i.setRGB(c++, d, 0xF0F0F0);i.setRGB(c++, d, 0xF0F0F0);i.setRGB(c++, d, 0xF0F0F0);i.setRGB(c++, d, 0xF0F0F0);   // 8x
            d++;kza++;
            } while (kza<=kko);
                                           }               
 //    g.clearRect(5, 275, 200, 20);
 //    g.drawString(tx, 5,290);           // zobraz pomocnou informaci
    }    // paint
}
