import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;

public class Leaderboard {
    private record Score(String name, int sections) {}

    public static void main(String[] args) throws IOException {
        Path scoreFile = Path.of("scores.txt");
        if (!Files.exists(scoreFile)) {
            System.out.println("No scores yet. Play the game first.");
            return;
        }

        List<Score> scores = new ArrayList<>();
        int lineNumber = 0;
        for (String line : Files.readAllLines(scoreFile)) {
            lineNumber++;
            int separator = line.lastIndexOf(',');
            if (separator <= 0 || separator == line.length() - 1) {
                throw new IOException("Invalid score entry on line " + lineNumber);
            }
            try {
                String name = line.substring(0, separator);
                int sections = Integer.parseInt(line.substring(separator + 1));
                scores.add(new Score(name, sections));
            } catch (NumberFormatException error) {
                throw new IOException("Invalid score entry on line " + lineNumber, error);
            }
        }

        scores.sort(Comparator.comparingInt(Score::sections).reversed());
        System.out.println("=== HIGH SCORES ===");
        for (int i = 0; i < scores.size(); i++) {
            Score score = scores.get(i);
            System.out.printf("%2d. %-20s %d sections%n",
                    i + 1, score.name(), score.sections());
        }
    }
}
