package xyz.suonan.myfolder_sever.Service.Executor;

import jakarta.annotation.PreDestroy;
import org.springframework.stereotype.Service;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.ThreadPoolExecutor;

@Service
public class FileTaskExecutor {
    private final ExecutorService executor = Executors.newFixedThreadPool(4);

    public void submit(Runnable task) {
        executor.submit(task);
    }

    public int getThreadActiveCount(){
        ThreadPoolExecutor  executorMinor= (ThreadPoolExecutor) executor;
        return executorMinor.getActiveCount();
    }

    @PreDestroy
    public void shutdown() {
        executor.shutdown();
    }
}