using Azure;
using Microsoft.CognitiveServices.Speech;
using MySqlX.XDevAPI.Common;
using System;
using System.Net.Sockets;
using System.Text;
using System.Text.Json;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;
using System.Windows.Media;

namespace Apayo
{
    public partial class MainWindow : Window
    {
        private SpeechRecognizer recognizer;
        private TcpClient client;
        private NetworkStream stream;

        public MainWindow()
        {
            InitializeComponent();
            ConnectToServer();

            var style = new Style(typeof(TabItem));
            style.Setters.Add(new Setter(TabItem.VisibilityProperty, Visibility.Collapsed));
            MyTabControl.ItemContainerStyle = style;

            IdTextBox.Text = "아이디를 입력해주세요";
            IdTextBox.Foreground = Brushes.Gray;

            // 이벤트 등록
            IdTextBox.GotFocus += IdTextBox_GotFocus;
            IdTextBox.LostFocus += IdTextBox_LostFocus;
        }
        private void IdTextBox_GotFocus(object sender, RoutedEventArgs e)
        {
            if (IdTextBox.Text == "아이디를 입력해주세요")
            {
                IdTextBox.Text = "";
                IdTextBox.Foreground = Brushes.Black; // 일반 텍스트 색으로 변경
            }
        }

        private void IdTextBox_LostFocus(object sender, RoutedEventArgs e)
        {
            if (string.IsNullOrWhiteSpace(IdTextBox.Text))
            {
                IdTextBox.Text = "아이디를 입력해주세요";
                IdTextBox.Foreground = Brushes.Gray; // 안내 문구는 회색으로 표시
            }
        }
        private void AppendOutput(string text)
        {
            Dispatcher.Invoke(() =>
            {
                
            });
        }

        private async void ConnectToServer()
        {
            try
            {
                client = new TcpClient();
                await client.ConnectAsync("10.10.20.109", 10000); // 서버 IP와 포트 조정
                stream = client.GetStream();
                
            }
            catch (Exception ex)
            {
                
            }
        }

        private async void SendTextToServer(string message)
        {
            if (stream == null || !client.Connected)
                return;

            try
            {
                byte[] data = Encoding.UTF8.GetBytes(message + "\n");
                await stream.WriteAsync(data, 0, data.Length);

                byte[] buffer = new byte[1024];
                int bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                string response = Encoding.UTF8.GetString(buffer, 0, bytesRead);
                MessageBox.Show("서버에서 온 원본 JSON:\n" + response);

                string[] messages = response.Split(new char[] { '\n' }, StringSplitOptions.RemoveEmptyEntries);

                foreach (string jsonStr in messages)
                {
                    JsonDocument doc = null;
                    try
                    {
                        doc = JsonDocument.Parse(jsonStr.Trim());
                        var root = doc.RootElement;

                        if (root.TryGetProperty("signal", out var signalProp) &&
                            signalProp.GetString() == "request")
                        {
                            string serverMessage = root.GetProperty("result").GetString();

                            Dispatcher.Invoke(() =>
                            {
                                AddChatBubble(serverMessage, isFromServer: true);
                            });
                        }
                    }
                    catch (Exception ex)
                    {
                        Console.WriteLine("JSON 파싱 오류: " + ex.Message);
                        Dispatcher.Invoke(() => AddChatBubble("[응답 파싱 오류]", true));
                    }
                    finally
                    {
                        doc?.Dispose();
                    }
                }
            }
            catch (Exception ex)
            {
                Dispatcher.Invoke(() =>
                {
                    AddChatBubble("[서버 통신 오류]", isFromServer: true);
                });
            }
        }


        private void AddChatBubble(string message, bool isFromServer)
        {
            var textBlock = new TextBlock
            {
                Text = message,
                TextWrapping = TextWrapping.Wrap,
                Margin = new Thickness(10),
                Background = isFromServer ? Brushes.LightBlue : Brushes.LightGray,
                Foreground = Brushes.Black,
                Padding = new Thickness(10),
                MaxWidth = 300
            };

            var border = new Border
            {
                CornerRadius = new CornerRadius(10),
                Background = textBlock.Background,
                Child = textBlock,
                HorizontalAlignment = isFromServer ? HorizontalAlignment.Right : HorizontalAlignment.Left,
                Margin = new Thickness(5)
            };

            ChatStackPanel.Children.Add(border);

            // 스크롤 맨 아래로 이동
            ChatScrollViewer.ScrollToEnd();
        }
        private void Recognizer_Recognized(object sender, SpeechRecognitionEventArgs e)
        {
            if (!string.IsNullOrEmpty(e.Result.Text))
            {
                Dispatcher.Invoke(() =>
                {
                    AddChatBubble(e.Result.Text, isFromServer: false); // 👈 왼쪽에 사용자 말 추가
                    OutputTextBox.Text = e.Result.Text;
                });

                var recordObj = new
                {
                    signal = "record",
                    say_text = e.Result.Text
                };

                string json = JsonSerializer.Serialize(recordObj);
                SendTextToServer(json); // 서버 응답 오면 아래에 오른쪽 말풍선으로 붙이기
            }
        }
        private void Recognizer_Canceled(object sender, SpeechRecognitionCanceledEventArgs e)
        {
            Dispatcher.Invoke(() =>
            {
                OutputTextBox.Text = $" 인식 취소: {e.Reason}";
            });
        }
        private void OutputTextBox_TextChanged(object sender, TextChangedEventArgs e)
        {
            
        }
        private void RecordButton_Click(object sender, RoutedEventArgs e)
        {
            
        }

        private void id_TextChanged(object sender, TextChangedEventArgs e)
        {

        }
        private void Join_Click(object sender, RoutedEventArgs e)
        {
            MyTabControl.SelectedIndex = 2;
        }

        private async void Login_Click(object sender, RoutedEventArgs e)
        {
            if (stream == null || !client.Connected)
            {
                AppendOutput(" 서버 연결이 없습니다.\n");
                return;
            }

            var loginObj = new
            {
                signal = "login",
                id = IdTextBox.Text.Trim(),
                pw = PasswordTextBox.Password
            };

            string json = JsonSerializer.Serialize(loginObj) + "\n";
            byte[] data = Encoding.UTF8.GetBytes(json);

            try
            {
                await stream.WriteAsync(data, 0, data.Length);
                AppendOutput($"➡ 로그인 요청 보냄: {json}");

                byte[] buffer = new byte[256];
                int bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                string response = Encoding.UTF8.GetString(buffer, 0, bytesRead).Trim();

                
                var doc = JsonDocument.Parse(response);
                var root = doc.RootElement;

                string signal = root.GetProperty("signal").GetString();
                string result = root.GetProperty("result").GetString();

                if (signal == "login_result")
                {
                    if (result == "0")
                    {
                        MessageBox.Show(" 로그인 성공");
                        MyTabControl.SelectedIndex = 3;
                    }
                    else if (result == "1")
                    {
                        MessageBox.Show(" 로그인 실패\n 아이디 또는 비밀번호를 확인해주세요.");
                    }
                }
                else
                {
                    MessageBox.Show($" 서버에서 예상치 못한 signal: {signal}");
                }

                doc.Dispose(); // 수동으로 메모리 해제
            }
            catch (Exception ex)
            {
                MessageBox.Show($" 서버 통신 중 오류: {ex.Message}");
            }

            IdTextBox.Text = "";
            PasswordTextBox.Clear();
            IdTextBox_LostFocus(null, null);
        }
        private void back_btn_Click(object sender, RoutedEventArgs e)
        {
            MyTabControl.SelectedIndex = 0;
        }
        private async void JoinButton_Click(object sender, RoutedEventArgs e)
        {
            if (stream == null || !client.Connected)
            {
                MessageBox.Show("❌ 서버 연결이 없습니다.");
                return;
            }

            var joinObj = new
            {
                signal = "join",
                id = join_IdTextBox.Text.Trim(),
                pw = join_PasswordTextBox.Password,
                adress = join_adress.Text.Trim(),
                phone = join_phone.Text.Trim(),
                gender = join_Gender.Text.Trim(),
                year = join_year.Text.Trim(),
                name = Join_name.Text.Trim(),

            };

            string json = JsonSerializer.Serialize(joinObj) + "\n";

            //Console.WriteLine($"보내는 JSON: {json}");
            //MessageBox.Show($"보내는 JSON:\n{json}");

            byte[] data = Encoding.UTF8.GetBytes(json);

            try
            {
                await stream.WriteAsync(data, 0, data.Length);
                //MessageBox.Show(" 회원가입 요청이 서버에 전송되었습니다.");

                // 서버 응답 받기 
                byte[] buffer = new byte[256];
                int bytesRead = await stream.ReadAsync(buffer, 0, buffer.Length);
                string response = Encoding.UTF8.GetString(buffer, 0, bytesRead).Trim();
                var doc = JsonDocument.Parse(response);
                var root = doc.RootElement;

                string signal = root.GetProperty("signal").GetString();
                string result = root.GetProperty("result").GetString();

                // 예: 서버가 0 = 성공, 1 = 실패 응답할 때 처리

                if (signal == "join_success")
                {
                    if (result == "0")
                    {
                        MessageBox.Show("회원가입 성공");
                        MyTabControl.SelectedIndex = 0;
                    }
                    else if (result == "1")
                    {
                        MessageBox.Show("회원가입 실패");
                        MyTabControl.SelectedIndex = 0;
                    }
                }
                else
                {
                    MessageBox.Show($" 알 수 없는 응답: {response}");
                }
            }
            catch (Exception ex)
            {
                MessageBox.Show($" 서버 통신 오류: {ex.Message}");
            }
        }
        private bool isRecognizing = false;

        private async void RecordButton_Click_1(object sender, RoutedEventArgs e)
        {
            if (!isRecognizing)
            {
                string apiKey = "6E7e5wt316M9WHV2yXRS99xxIlhBT8lM1CcxQzUpvsSG2wGl3PL3JQQJ99BGACNns7RXJ3w3AAAYACOGPtwI";
                string region = "koreacentral";

                var config = SpeechConfig.FromSubscription(apiKey, region);
                config.SpeechRecognitionLanguage = "ko-KR";

                recognizer = new SpeechRecognizer(config);

                recognizer.Recognized += Recognizer_Recognized;
                recognizer.Canceled += Recognizer_Canceled;

                await recognizer.StartContinuousRecognitionAsync();

                //OutputTextBox.Text = "말하세요...";
                //RecordButton.Content = "중지"; // 
                isRecognizing = true;
            }
            else
            {
                if (recognizer != null)
                {
                    await recognizer.StopContinuousRecognitionAsync();

                    recognizer.Recognized -= Recognizer_Recognized;
                    recognizer.Canceled -= Recognizer_Canceled;
                    recognizer.Dispose();
                    recognizer = null;

                    //OutputTextBox.Text = "인식 종료됨";
                    //RecordButton.Content = "시작"; // 
                    isRecognizing = false;
                }
            }
        }

        private void Button_Click_1(object sender, RoutedEventArgs e)
        {
            MyTabControl.SelectedIndex = 1;
        }
    }
}
