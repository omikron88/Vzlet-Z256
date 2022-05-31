/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package machine;

import java.io.*;
import java.util.logging.Level;
import java.util.logging.Logger;

public final class Memory {

    public final int PAGE_SIZE = 1024;
    public final int PAGE_MASK = PAGE_SIZE - 1;
    public final byte PAGE_BIT = 10;
    
    private byte[][] Ram = new byte[256][PAGE_SIZE];
    private byte[][] SRam = new byte[1][PAGE_SIZE];
    private byte[][] VRam = new byte[128][PAGE_SIZE];
    private byte[][] Boot = new byte[8][PAGE_SIZE];
    private byte[][] Video = new byte[8][PAGE_SIZE];
    
    private byte[] fakeRAM = new byte[PAGE_SIZE];  // contains FFs for no memory
    private byte[] nullRAM = new byte[PAGE_SIZE];  // for no mem writes
    private byte[][] readMonitor = new byte[16][];
    private byte[][] writeMonitor = new byte[16][];
    
    private Config cf;
    
    public Memory(Config cnf) {
        cf = cnf;
        
        loadRoms();
    }

    public void reset(boolean dirty) {
        if (dirty) {
            char c = 0;
            int a = 0;
            for(int i=0; i<256; i++) {
                for(int j=0; j<PAGE_SIZE; j++) {
                    Ram[i][j] = (byte) c;
                    if (i<128) {VRam[i][j] = (byte) c;}
                    a = (a+1) & 0X7f;
                    if (a==0) { c ^= 0xff; };
                }
            }
        }
  
        for(int i=0; i<PAGE_SIZE; i++) {
                fakeRAM[i] = (byte) 0xff;   
            }

        readMonitor[0] = Boot[0]; writeMonitor[0] = nullRAM; 
        readMonitor[1] = Boot[1]; writeMonitor[1] = nullRAM; 
        readMonitor[2] = Boot[2]; writeMonitor[2] = nullRAM; 
        readMonitor[3] = Boot[3]; writeMonitor[3] = nullRAM; 
        readMonitor[4] = Boot[4]; writeMonitor[4] = nullRAM; 
        readMonitor[5] = Boot[5]; writeMonitor[5] = nullRAM; 
        readMonitor[6] = Boot[6]; writeMonitor[6] = nullRAM; 
        readMonitor[7] = Boot[7]; writeMonitor[7] = nullRAM; 
        readMonitor[8] = SRam[0]; writeMonitor[8] = SRam[0]; 
        readMonitor[9] = SRam[0]; writeMonitor[9] = SRam[0]; 
        readMonitor[10] = SRam[0]; writeMonitor[10] = SRam[0]; 
        readMonitor[11] = SRam[0]; writeMonitor[11] = SRam[0]; 
        readMonitor[12] = SRam[0]; writeMonitor[12] = SRam[0]; 
        readMonitor[13] = SRam[0]; writeMonitor[13] = SRam[0]; 
        readMonitor[14] = SRam[0]; writeMonitor[14] = SRam[0]; 
        readMonitor[15] = SRam[0]; writeMonitor[15] = SRam[0];
        loadRamD();
    }
    
    public void dumpRam(String fname, int first, int last) {
        RandomAccessFile f;
        try {
            f = new RandomAccessFile(fname, "rw");
            for(int i=first; i<(last+1); i++) {
                f.write(Ram[i]);
            }
            f.close();
        } catch (IOException ex) {
            Logger.getLogger(Memory.class.getName()).log(Level.SEVERE, null, ex);
        }
        
    }
    
    public byte readMonitor(int address) {
        return readMonitor[address >>> PAGE_BIT][address & PAGE_MASK];
    }
    
    public void writeMonitor(int address, byte value) {
        writeMonitor[address >>> PAGE_BIT][address & PAGE_MASK] = value;
    }
          
    public byte readRam(int address) {
        return Ram[address >>> PAGE_BIT][address & PAGE_MASK];
    }
    
    public void writeRam(int address, byte value) {
        Ram[address >>> PAGE_BIT][address & PAGE_MASK] = value;
    }
          
    public byte readVram(int address) {
        return VRam[address >>> PAGE_BIT][address & PAGE_MASK];
    }
    
    public void writeVram(int address, byte value) {
        VRam[address >>> PAGE_BIT][address & PAGE_MASK] = value;
    }
          
    public byte readVideo(int address) {
        return Video[address >>> PAGE_BIT][address & PAGE_MASK];
    }
    
    private void loadRoms() {
        if (!loadRomAsFile("roms/z.bin", Boot, 0, PAGE_SIZE*8)) {
            loadRomAsResource("/roms/z.bin", Boot, 0, PAGE_SIZE*8);
        }
        if (!loadRomAsFile("roms/video.bin", Video, 0, PAGE_SIZE*2)) {
            loadRomAsResource("/roms/video.bin", Video, 0, PAGE_SIZE*2);
        }
    }
     private void loadRamD() {
        if (!loadRomAsFile("roms/rd.z2r", Ram, 64, PAGE_SIZE*181)) {
            loadRomAsResource("/roms/rd.z2r", Ram, 64, PAGE_SIZE*181);
        }
     }
    private boolean loadRomAsResource(String filename, byte[][] rom, int page, int size) {

        InputStream inRom = Vzlet.class.getResourceAsStream(filename);
        boolean res = false;

        if (inRom == null) {
            String msg =
                java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                "RESOURCE_ROM_ERROR");
            System.out.println(String.format("%s: %s", msg, filename));
            return false;
        }

        try {
            for (int frag = 0; frag < size / PAGE_SIZE; frag++) {
                int count = 0;
                while (count != -1 && count < PAGE_SIZE) {
                    count += inRom.read(rom[page + frag], count, PAGE_SIZE - count);
                }

                if (count != PAGE_SIZE) {
                    String msg =
                        java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                        "ROM_SIZE_ERROR");
                    System.out.println(String.format("%s: %s", msg, filename));
                } else {
                    res = true;
                }
            }
        } catch (IOException ex) {
            String msg =
                java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                "RESOURCE_ROM_ERROR");
            System.out.println(String.format("%s: %s", msg, filename));
            Logger.getLogger(Memory.class.getName()).log(Level.SEVERE, null, ex);
        } finally {
            try {
                inRom.close();
            } catch (IOException ex) {
                Logger.getLogger(Memory.class.getName()).log(Level.SEVERE, null, ex);
            }
        }

        if (res) {
            String msg =
                java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                "ROM_RESOURCE_LOADED");
            System.out.println(String.format("%s: %s", msg, filename));
        }

        return res;
    }

    private boolean loadRomAsFile(String filename, byte[][] rom, int page, int size) {
        BufferedInputStream fIn = null;
        boolean res = false;

        try {
            try {
                fIn = new BufferedInputStream(new FileInputStream(filename));
            } catch (FileNotFoundException ex) {
                String msg =
                    java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                    "FILE_ROM_ERROR");
                System.out.println(String.format("%s: %s", msg, filename));
                //Logger.getLogger(Spectrum.class.getName()).log(Level.SEVERE, null, ex);
                return false;
            }

            for (int frag = 0; frag < size / PAGE_SIZE; frag++) {
                int count = 0;
                while (count != -1 && count < PAGE_SIZE) {
                    count += fIn.read(rom[page + frag], count, PAGE_SIZE - count);
                }

                if (count != PAGE_SIZE) {
                    String msg =
                        java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                        "ROM_SIZE_ERROR");
                    System.out.println(String.format("%s: %s", msg, filename));
                } else {
                    res = true;
                }
            }
        } catch (IOException ex) {
            String msg =
                java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                "FILE_ROM_ERROR");
            System.out.println(String.format("%s: %s", msg, filename));
            Logger.getLogger(Memory.class.getName()).log(Level.SEVERE, null, ex);
        } finally {
            try {
                if (fIn != null) {
                    fIn.close();
                }
            } catch (IOException ex) {
                Logger.getLogger(Memory.class.getName()).log(Level.SEVERE, null, ex);
            }
        }

        if (res) {
            String msg =
                java.util.ResourceBundle.getBundle("machine/Bundle").getString(
                "ROM_FILE_LOADED");
            System.out.println(String.format("%s: %s", msg, filename));
        }

        return res;
    }

}
