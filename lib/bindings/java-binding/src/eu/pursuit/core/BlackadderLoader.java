package eu.pursuit.core;

import java.io.File;
import java.util.concurrent.atomic.AtomicBoolean;

public class BlackadderLoader {
	private static AtomicBoolean soLoaded = new AtomicBoolean(false);
	
	public synchronized static void loadSo(){
		if(soLoaded.compareAndSet(false, true)){
			String so_path = "/usr/local/lib/libblackadder_java_binding.so";
			File so = new File(so_path);
			if(!so.exists() || !so.isFile()){
				String mesg = "cannot find file "+ so_path + ". Have you installed the binding jni part with \"(sudo) make install\"?";
				System.out.println(mesg);
				throw new RuntimeException(mesg);
			}
			
			System.load(so_path);
		}
	}
}
