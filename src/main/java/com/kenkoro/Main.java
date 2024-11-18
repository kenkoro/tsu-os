package com.kenkoro;

class Monitor {
  private static volatile boolean ready = false;

  public synchronized void produce() {
    if (ready) return;
    try {
      Thread.sleep(1000L);
    } catch (InterruptedException e) {
      System.out.println(e.getMessage());
    }
    ready = true;
    System.out.println("Produced");
    notify();
  }

  public synchronized void consume() {
    try {
      while (!ready) wait();
    } catch (InterruptedException e) {
      System.out.println(e.getMessage());
    }
    ready = false;
    System.out.println("Consumed");
  }
}

public class Main {
  public static void main(String[] args) {
    var monitor = new Monitor();
    new Thread(() -> {
      while (true) {
        monitor.produce();
      }
    }).start();
    new Thread(() -> {
      while (true) {
        monitor.consume();
      }
    }).start();
  }
}