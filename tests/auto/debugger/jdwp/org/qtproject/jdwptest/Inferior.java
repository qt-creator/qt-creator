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
    }

    static int square(int value)
    {
        int result = value * value; // square-body
        return result; // square-return
    }

    static int sum(int[] values)
    {
        int total = 0;
        for (int value : values)
            total += value; // sum-loop
        return total;
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
        int number = 7; // main-start
        String text = "hello";
        int[] values = {1, 2, 3};

        // no-code
        Point point = new Point(3, 4); // after-no-code
        int squared = square(number); // call-square
        int total = sum(values); // after-square
        Helper.Nested nested = new Helper.Nested(total);
        int described = nested.describe(); // call-nested
        System.out.println("result " + squared + " " + total + " " + described + " " + text
                           + " " + point.x); // print-result
        System.exit(3);
    }
}
