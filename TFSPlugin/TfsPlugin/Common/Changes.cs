using System.Collections.Generic;

namespace TfsPlugin
{
    public class ChangelistRevision : IReadsFromConnection
    {
        public readonly static string kDefaultListRevision = "-1";
        public readonly static string kNewListRevision = "-2";

        public string Value { get; set; }

        public ChangelistRevision()
        {

        }

        public ChangelistRevision(string value)
        {
            this.Value = value;
        }

        public static implicit operator string(ChangelistRevision p)
        {
            return p.Value;
        }

        public static implicit operator ChangelistRevision(string path)
        {
            return new ChangelistRevision(path);
        }

        public void Read(Connection p)
        {
            Value = p.ReadLine();
        }

        public int ToInt()
        {
            return int.Parse(Value);
        }
    }

    public class Changelist : IReadsFromConnection, IWritesToConnection
    {
        public Changelist()
        {
            Revision = string.Empty;
            Description = string.Empty;
            Timestamp = string.Empty;
            Committer = string.Empty;
        }

        public ChangelistRevision GetRevision() { return Revision; }
        public void SetRevision(ChangelistRevision revison) { Revision = revison; }

        public string GetDescription() { return Description; }
        public void SetDescription(string description) { Description = description; }

        public string GetTimestamp() { return Timestamp; }
        public void SetTimestamp(string timestamp) { Timestamp = timestamp; }

        public string GetCommitter() { return Committer; }
        public void SetCommitter(string committer) { Committer = committer; }

        public ChangelistRevision Revision { get; set; }
        public string Description { get; set; }
        public string Timestamp { get; set; }
        public string Committer { get; set; }

        public void Write(Connection p)
        {
            p.DataLine(GetRevision());
            p.DataLine(GetDescription());
        }

        public void Read(Connection p)
        {
            SetRevision(p.ReadLine());
            SetDescription(p.ReadLine());
        }
    }

    public class ChangelistRevisions : List<ChangelistRevision>
    {

    }
}
