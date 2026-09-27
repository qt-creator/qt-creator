// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

package org.qtproject.jdwptest;

// The lines the test stops at are found by the markers in their comments.
public class Inferior
{
    static class Point
    {
        final int x;
        final int y;

        Point(int x, int y)
        {
            this.x = x;
            this.y = y;
        }

        @Override
        public String toString()
        {
            return "(" + x + ", " + y + ")"; // tostring-body
        }
    }

    // No toString() of its own, and none inherited either: what it shows is
    // the identity java.lang.Object answers with.
    static class Plain
    {
        int kept = 5;
    }

    // What it answers would break out of the one line it is shown on.
    static class Wordy
    {
        @Override
        public String toString()
        {
            return "one\ttwo\nthree";
        }
    }

    // A toString() that does not come back, which the debugger has to give up
    // on rather than wait for.
    static class Slow
    {
        @Override
        public String toString()
        {
            try {
                Thread.sleep(300000);
            } catch (InterruptedException ignored) {
            }
            return "late";
        }
    }

    static int slow()
    {
        Slow first = new Slow();
        Slow second = new Slow();
        return first.hashCode() + second.hashCode(); // slow-body
    }

    // A toString() that does not come back, and that runs into a breakpoint
    // over and over while it does not.
    static class Looping
    {
        @Override
        public String toString()
        {
            while (true) {
                spins = spins + 1; // looping-body
                try {
                    Thread.sleep(20);
                } catch (InterruptedException ignored) {
                }
            }
        }
    }

    // A second thread with something of its own to show, which is running,
    // and therefore has frames to read, when the machine is suspended.
    static int looping()
    {
        Thread worker = new Thread(() -> {
            Point seen = new Point(7, 8);
            spinning = true;
            while (!released)
                spins = spins + 1;
            spins = spins + seen.y;
        });
        worker.setName("worker");
        worker.setDaemon(true);
        worker.start();
        while (!spinning) {
            Thread.yield();
        }
        Looping looping = new Looping();
        return looping.hashCode(); // looping-stop
    }

    // Throws where it is told to, which is how the exception breakpoint has
    // something to stop at.
    static void trouble(boolean handled)
    {
        if (handled) {
            try {
                throw new IllegalStateException("caught"); // throw-caught
            } catch (IllegalStateException ignored) {
            }
            return;
        }
        throw new IllegalArgumentException("loose"); // throw-uncaught
    }

    static int calls = 0;

    // What the threads of looping() go by. Nothing ever releases the worker:
    // it spins until the program is killed, which is what leaves it with
    // frames of its own to read. They are written here, below every class
    // that uses them, so that the lines of the static initializer do not run
    // around those of a class it knows nothing of: a breakpoint that finds no
    // line of its own in a class moves to the next line of a method whose
    // lines lie around it.
    static volatile boolean spinning = false;
    static volatile boolean released = false;
    static int spins = 0;

    // A string that is not plain ASCII: a null character, and one outside the
    // basic plane, written here as the halves of its pair.
    static String awkward = "a\0b\ud83d\ude00c";

    // Negative numbers, which the wire carries in as few bytes as they take.
    static byte tiny = -8;
    static short small = -300;
    static int negative = -5;
    static long least = Long.MIN_VALUE;

    static int square(int value)
    {
        int result = value * value; // square-body
        return result; // square-return
    }

    static int sum(int[] values)
    {
        calls = calls + 1;
        int total = 0;
        for (int value : values)
            total += value; // sum-loop
        return total;
    }

    // Two objects that say what they are, so that one call has to wait for the
    // other, and one that has nothing to say.
    static int show()
    {
        Point located = new Point(3, 4);
        Point other = new Point(5, 6);
        Plain plain = new Plain();
        Wordy wordy = new Wordy();
        return located.x + other.y + plain.kept + wordy.hashCode(); // show-body
    }

    static void spin()
    {
        long counter = 0;
        while (counter >= 0)
            counter = counter + 1; // spin-body
    }

    public static void main(String[] args)
    {
        if (args.length > 0 && args[0].equals("spin")) {
            spin();
            return;
        }
        if (args.length > 0 && args[0].equals("show")) {
            show();
            return;
        }
        if (args.length > 0 && args[0].equals("throw")) {
            trouble(true);
            trouble(false);
            return;
        }
        if (args.length > 0 && args[0].equals("slow")) {
            slow();
            return;
        }
        if (args.length > 0 && args[0].equals("looping")) {
            looping();
            return;
        }
        if (args.length > 0 && args[0].equals("swallow")) {
            // Integer.getInteger() decodes the property and keeps the
            // NumberFormatException that comes of it to itself, so the throw
            // and the catch are both inside java.lang.Integer. The runtime
            // does this by the dozen while it loads classes and looks up
            // character sets; this one is here to be sure of having one.
            System.setProperty("org.qtproject.jdwptest.number", "not a number");
            Integer.getInteger("org.qtproject.jdwptest.number", 7);
            trouble(false);
            return;
        }
        if (args.length > 0 && args[0].equals("parse")) {
            // Thrown inside the runtime rather than here, which is where most
            // of what a debugger is wanted for comes from.
            Integer.parseInt("not a number");
            return;
        }
        int number = 7; // main-start
        String text = "hello";
        int[] values = {1, 2, 3};

        // no-code
        Point point = new Point(3, 4); // after-no-code
        int squared = square(number); // call-square
        int total = sum(values); // after-square
        Helper.Nested nested = new Helper.Nested(total);
        int described = nested.describe(); // call-nested
        nested.shadow();
        System.out.println("result " + squared + " " + total + " " + described + " " + text
                           + " " + point.x); // print-result
        System.exit(3);
    }
}
