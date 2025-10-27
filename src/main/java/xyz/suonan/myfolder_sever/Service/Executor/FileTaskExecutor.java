package xyz.suonan.myfolder_sever.Service.Executor;

import jakarta.annotation.PreDestroy;
import org.springframework.stereotype.Service;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

@Service
public class FileTaskExecutor {
    private final ExecutorService executor = Executors.newFixedThreadPool(4);

    public void submit(Runnable task) {
        executor.submit(task);
    }

    @PreDestroy
    public void shutdown() {
        executor.shutdown();
    }
}